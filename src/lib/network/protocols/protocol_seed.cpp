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
#include <metaverse/network/protocols/protocol_seed.hpp>

#include <functional>
#include <metaverse/bitcoin.hpp>
#include <metaverse/network/channel.hpp>
#include <metaverse/network/p2p.hpp>
#include <metaverse/network/protocols/protocol_timer.hpp>

namespace libbitcoin {
namespace network {

#define NAME "seed"
#define CLASS protocol_seed

using namespace bc::message;
using namespace std::placeholders;

// Require three callbacks (or any error) before calling complete.
protocol_seed::protocol_seed(p2p& network, channel::ptr channel)
  : protocol_timer(network, channel, false, NAME),
    network_(network),
    CONSTRUCT_TRACK(protocol_seed)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_seed::start(event_handler handler)
{
    const auto& settings = network_.network_settings();

	auto complete = [=, self = shared_from_base<protocol_seed>()](const code& ec)
	{
		return self->handle_seeding_complete(ec, handler);
	};

    if (settings.host_pool_capacity == 0)
    {
        complete(error::not_found);
        return;
    }

    protocol_timer::start(settings.channel_germination(),
        synchronize(complete, 3, NAME, false));

	subscribe<address>([self = shared_from_base<protocol_seed>()]
		(const code& ec, address::ptr message) {
			return self->handle_receive_address(ec, message);
		});
    send_own_address(settings);
	send(get_address(), [self = shared_from_base<protocol_seed>()]
		(const code& ec) {
			return self->handle_send_get_address(ec);
		});
}

// Protocol.
// ----------------------------------------------------------------------------

void protocol_seed::send_own_address(const settings& settings)
{
    if (settings.self.port() == 0)
    {
        set_event(error::success);
        return;
    }

    const address self({ { settings.self.to_network_address() } });
	send(self, [self = shared_from_base<protocol_seed>()]
		(const code& ec) {
			return self->handle_send_address(ec);
		});
}

void protocol_seed::handle_seeding_complete(const code& ec,
    event_handler handler)
{
    handler(ec);
    stop(ec);
}

bool protocol_seed::handle_receive_address(const code& ec,
    address::ptr message)
{
    if (stopped())
        return false;

    if (ec)
    {
        log::trace(LOG_NETWORK)
            << "Failure receiving addresses from seed [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return false;
    }

    log::trace(LOG_NETWORK)
        << "Storing addresses from seed [" << authority() << "] ("
        << message->addresses.size() << ")";

    // TODO: manage timestamps (active channels are connected < 3 hours ago).
	auto handler = [self = shared_from_base<protocol_seed>()]
		(const code& ec) {
			return self->handle_store_addresses(ec);
		};
    network_.store(message->addresses, handler);

    return false;
}

void protocol_seed::handle_send_address(const code& ec)
{
    if (stopped())
        return;

    if (ec)
    {
        log::trace(LOG_NETWORK)
            << "Failure sending address to seed [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return;
    }

    // 1 of 3
    set_event(error::success);
}

void protocol_seed::handle_send_get_address(const code& ec)
{
    if (stopped())
        return;

    if (ec)
    {
        log::trace(LOG_NETWORK)
            << "Failure sending get_address to seed [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return;
    }

    // 2 of 3
    set_event(error::success);
}

void protocol_seed::handle_store_addresses(const code& ec)
{
    if (stopped())
        return;

    if (ec)
    {
        log::error(LOG_NETWORK)
            << "Failure storing addresses from seed [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return;
    }

    log::trace(LOG_NETWORK)
        << "Stopping completed seed [" << authority() << "] ";

    // 3 of 3
    set_event(error::channel_stopped);
}

} // namespace network
} // namespace libbitcoin
