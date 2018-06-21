/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 * Copyright (c) 2016-2018 metaverse core developers (see MVS-AUTHORS)
 *
 * This file is part of metaverse.
 *
 * metaverse is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MVS_NETWORK_PROTOCOL_HPP
#define MVS_NETWORK_PROTOCOL_HPP

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <metaverse/bitcoin.hpp>
#include <metaverse/network/channel.hpp>
#include <metaverse/network/define.hpp>

namespace libbitcoin {
namespace network {

class p2p;

/// Virtual base class for protocol implementation, mostly thread safe.
class BCT_API protocol
  : public enable_shared_from_base<protocol>
{
protected:
    typedef std::shared_ptr<protocol> ptr;
    typedef std::function<void()> completion_handler;
    typedef std::function<void(const code&)> event_handler;
    typedef std::function<void(const code&, size_t)> count_handler;

    /// Construct an instance.
    protocol(p2p& network, channel::ptr channel, const std::string& name);

    /// This class is not copyable.
    protocol(const protocol&) = delete;
    void operator=(const protocol&) = delete;

    /// Send a message on the channel and handle the result.
	template <class Message, typename Handler>
	void send(Message&& packet, Handler&& handler)
	{
		channel_->send(std::forward<Message>(packet), handler);
	}
    /// Subscribe to all channel messages, blocking until subscribed.
	template <class Message, typename Handler>
	void subscribe(Handler&& handler)
	{
		channel_->template subscribe<Message>(handler);
	}

    /// Subscribe to the channel stop, blocking until subscribed.
    template <typename Handler>
    void subscribe_stop(Handler&& handler)
    {
        channel_->subscribe_stop(handler);
    }

    /// Get the address of the channel.
    virtual config::authority authority() const;

    /// Get the protocol name, for logging purposes.
    virtual const std::string& name() const;

    /// Get the channel nonce.
    virtual uint64_t nonce() const;

    /// Get the peer version message. This method is NOT thread safe and must
    /// not be called if any other thread could write the peer version.
    virtual message::version peer_version() const;

    /// Set the channel version. This method is NOT thread safe and must
    /// complete before any other thread could read the peer version.
    virtual void set_peer_version(message::version::ptr value);

    uint32_t peer_start_height();

    /// Get the threadpool.
    virtual threadpool& pool();

    /// Stop the channel (and the protocol).
    virtual void stop(const code& ec);

    virtual bool misbehaving(int32_t howmuch);

    bool channel_stopped() { return channel_->stopped(); }

private:
    threadpool& pool_;
    channel::ptr channel_;
    const std::string name_;
};

} // namespace network
} // namespace libbitcoin

#endif
