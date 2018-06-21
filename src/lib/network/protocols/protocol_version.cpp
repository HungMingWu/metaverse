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
#include <metaverse/network/protocols/protocol_version.hpp>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <metaverse/bitcoin.hpp>
#include <metaverse/network/channel.hpp>
#include <metaverse/network/p2p.hpp>
#include <metaverse/network/protocols/protocol_timer.hpp>
#include <metaverse/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "version"
#define CLASS protocol_version

using namespace bc::message;
using namespace std::placeholders;

// TODO: move to libbitcoin utility with similar blockchain function.
static uint64_t time_stamp()
{
    // Use system clock because we require accurate time of day.
    typedef std::chrono::system_clock wall_clock;
    const auto now = wall_clock::now();
    return wall_clock::to_time_t(now);
}

message::version protocol_version::version_factory(
    const config::authority& authority, const settings& settings,
    uint64_t nonce, size_t height)
{
    BITCOIN_ASSERT_MSG(height < max_uint32, "Time to upgrade the protocol.");
    const auto height32 = static_cast<uint32_t>(height);

    // TODO: move services to authority member in base protocol (passed in).
    auto self = authority.to_network_address();
    self.services = services::node_network;

    return
    {
        settings.protocol,
        self.services,
        time_stamp(),
        self,
        settings.self.to_network_address(),
        nonce,
        BC_USER_AGENT,
        height32,
        settings.relay_transactions
    };
}

protocol_version::protocol_version(p2p& network, channel::ptr channel)
  : protocol_timer(network, channel, false, NAME),
    network_(network),
    CONSTRUCT_TRACK(protocol_version)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_version::start(event_handler handler)
{
    const auto height = network_.height();
    const auto& settings = network_.network_settings();
    complete_handler_ = handler;

    // The handler is invoked in the context of the last message receipt.
	auto handler1 = [self = shared_from_base<protocol_version>()]
		(const code& ec) {
			return self->handle_complete(ec);
		};
    protocol_timer::start(settings.channel_handshake(),
        synchronize(handler1, 2, NAME, false));

	subscribe<version>([self = shared_from_base<protocol_version>()]
		(const code& ec, version::ptr message) {
			return self->handle_receive_version(ec, message);
		});
	subscribe<verack>([self = shared_from_base<protocol_version>()]
		(const code& ec, verack::ptr message) {
			return self->handle_receive_verack(ec, message);
		});
    send_version(version_factory(authority(), settings, nonce(), height));
}

void protocol_version::handle_complete(const code& ec)
{
	if (!complete_handler_)
		return;
	complete_handler_(ec);
	complete_handler_ = nullptr;
}

void protocol_version::send_version(const message::version& self)
{
	send(self, [self = shared_from_base<protocol_version>()]
		(const code& ec) {
			return self->handle_version_sent(ec);
		});
}

// Protocol.
// ----------------------------------------------------------------------------

bool protocol_version::handle_receive_version(const code& ec,
    version::ptr message)
{
    if (stopped())
        return false;

    if (ec)
    {
        log::trace(LOG_NETWORK)
            << "Failure receiving version from [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return false;
    }

    log::trace(LOG_NETWORK)
        << "Peer [" << authority() << "] version (" << message->value
        << ") services (" << message->services << ") time ("
        << message->timestamp << ") " << message->user_agent;

    set_peer_version(message);
	send(verack(), [self = shared_from_base<protocol_version>()]
		(const code& ec) {
			return self->handle_verack_sent(ec);
		});

    // 1 of 2
    set_event(error::success);
    return false;
}

bool protocol_version::handle_receive_verack(const code& ec, verack::ptr)
{
    if (stopped())
        return false;

    if (ec)
    {
        log::trace(LOG_NETWORK)
            << "Failure receiving verack from [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return false;
    }

    // 2 of 2
    set_event(error::success);
    return false;
}

void protocol_version::handle_version_sent(const code& ec)
{
    if (stopped())
        return;

    if (ec)
    {
        log::trace(LOG_NETWORK)
            << "Failure sending version to [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return;
    }
}

void protocol_version::handle_verack_sent(const code& ec)
{
    if (stopped())
        return;

    if (ec)
    {
        log::trace(LOG_NETWORK)
            << "Failure sending verack to [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return;
    }
}

} // namespace network
} // namespace libbitcoin
