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
#include <metaverse/network/sessions/session_seed.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <metaverse/bitcoin.hpp>
#include <metaverse/network/p2p.hpp>
#include <metaverse/network/protocols/protocol_ping.hpp>
#include <metaverse/network/protocols/protocol_seed.hpp>
#include <metaverse/network/proxy.hpp>
#include <metaverse/bitcoin/config/authority.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_seed
#define NAME "session_seed"

using namespace std::placeholders;
session_seed::session_seed(p2p& network)
  : session(network, true, false),
    CONSTRUCT_TRACK(session_seed),
	network_{network}
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void session_seed::start(result_handler handler)
{
    if (settings_.host_pool_capacity == 0)
    {
        log::info(LOG_NETWORK)
            << "Not configured to populate an address pool.";
        handler(error::success);
        return;
    }

	std::function<void(const code&)> handle_started = [handler, self = shared_from_base<session_seed>()]
		(const code& ec) {
			return self->handle_started(ec, handler);
		};
    session::start(concurrent_delegate(handle_started));
}

void session_seed::restart(result_handler handler)
{
    handle_started(error::success, handler);
}

void session_seed::handle_started(const code& ec, result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

	auto handle_count = [=, self = shared_from_base<session_seed>()](size_t start_size) {
		return self->handle_count(start_size, handler);
	};
	address_count(handle_count);
}

void session_seed::handle_count(size_t start_size, result_handler handler)
{
    if (start_size != 0)
    {
        log::debug(LOG_NETWORK)
            << "Seeding is not required because there are " 
            << start_size << " cached addresses.";
        handler(error::success);
        return;
    }

    if (settings_.seeds.empty())
    {
        log::error(LOG_NETWORK)
            << "Seeding is required but no seeds are configured.";
        handler(error::operation_failed);
        return;
    }
    
    // This is NOT technically the end of the start sequence, since the handler
    // is not invoked until seeding operations are complete.
    start_seeding(start_size, create_connector(), handler);
}

// Seed sequence.
// ----------------------------------------------------------------------------

void session_seed::start_seeding(size_t start_size, SharedConnector connect,
    result_handler handler)
{
    // When all seeds are synchronized call session_seed::handle_complete.
	auto all = [=, self = shared_from_base<session_seed>()](const code& ec) {
		return self->handle_complete(start_size, handler);
	};
    // Synchronize each individual seed before calling handle_complete.
    auto each = synchronize(all, settings_.seeds.size(), NAME, true);

    // We don't use parallel here because connect is itself asynchronous.
    for (const auto& seed: settings_.seeds)
        start_seed(seed, connect, each);
}

void session_seed::start_seed(const config::endpoint& seed,
    SharedConnector connect, result_handler handler)
{
    if (stopped())
    {
        log::debug(LOG_NETWORK)
            << "Suspended seed connection";
        handler(error::channel_stopped);
        return;
    }

    log::info(LOG_NETWORK)
        << "Contacting seed [" << seed << "]";

    // OUTBOUND CONNECT
	auto handle_connect = [=, self = shared_from_base<session_seed>()]
		(const code& ec, channel::ptr channel) {
			return self->handle_connect(ec, channel, seed, handler);
		};
    connect->connect(seed, handle_connect, [this](const asio::endpoint& endpoint){
    	network_.store(config::authority{endpoint}.to_network_address(), [](const code& ec){});
    	log::debug(LOG_NETWORK) << "session seed store," << endpoint ;
    });
}

void session_seed::handle_connect(const code& ec, channel::ptr channel,
    const config::endpoint& seed, result_handler handler)
{
    if (ec)
    {
        log::info(LOG_NETWORK)
            << "Failure contacting seed [" << seed << "] " << ec.message();
        handler(ec);
        return;
    }

    if (blacklisted(channel->authority()))
    {
        log::debug(LOG_NETWORK)
            << "Seed [" << seed << "] on blacklisted address ["
            << channel->authority() << "]";
        handler(error::address_blocked);
        return;
    }

    log::info(LOG_NETWORK)
        << "Connected seed [" << seed << "] as " << channel->authority();

	auto handle_started = [=, self = shared_from_base<session_seed>()]
		(const code& ec) {
			return self->handle_channel_start(ec, channel, handler);
		};
	auto handle_stopped = [self = shared_from_base<session_seed>()]
		(const code& ec) {
			return self->handle_channel_stop(ec);
		};
    register_channel(channel, 
		handle_started,
		handle_stopped);
}

void session_seed::handle_channel_start(const code& ec, channel::ptr channel,
    result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

    attach_protocols(channel, handler);
};

void session_seed::attach_protocols(channel::ptr channel,
    result_handler handler)
{
    attach<protocol_ping>(channel)->start();
    attach<protocol_seed>(channel)->start(handler);
}

void session_seed::handle_channel_stop(const code& ec)
{
    log::info(LOG_NETWORK)
        << "Seed channel stopped: " << ec.message();
}

// This accepts no error code because individual seed errors are suppressed.
void session_seed::handle_complete(size_t start_size, result_handler handler)
{
	auto handle_final_count = [=, self = shared_from_base<session_seed>()](size_t current_size) {
		return self->handle_final_count(current_size, start_size, handler);
	};
    address_count(handle_final_count);

    log::info(LOG_NETWORK)
            << "session_seed complete!";
}

// We succeed only if there is a host count increase.
void session_seed::handle_final_count(size_t current_size, size_t start_size,
    result_handler handler)
{
    const auto result = current_size > start_size ? error::success :
        error::operation_failed;

    // This is the end of the seed sequence.
    handler(result);
}

} // namespace network
} // namespace libbitcoin
