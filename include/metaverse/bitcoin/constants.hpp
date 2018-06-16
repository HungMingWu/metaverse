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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MVS_CONSTANTS_HPP
#define MVS_CONSTANTS_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <metaverse/bitcoin/define.hpp>
#include <metaverse/bitcoin/message/network_address.hpp>
#include <metaverse/bitcoin/math/hash_number.hpp>

namespace libbitcoin {

#define BC_USER_AGENT "/metaverse:" MVS_VERSION "/"

// Generic constants.

constexpr size_t command_size = 12;

constexpr int64_t min_int64 = (std::numeric_limits<int64_t>::min)();
constexpr int64_t max_int64 = (std::numeric_limits<int64_t>::max)();
constexpr int32_t min_int32 = (std::numeric_limits<int32_t>::min)();
constexpr int32_t max_int32 = (std::numeric_limits<int32_t>::max)();
constexpr uint64_t max_uint64 = (std::numeric_limits<uint64_t>::max)();
constexpr uint32_t max_uint32 = (std::numeric_limits<uint32_t>::max)();
constexpr uint16_t max_uint16 = (std::numeric_limits<uint16_t>::max)();
constexpr uint8_t max_uint8 = (std::numeric_limits<uint8_t>::max)();
constexpr uint64_t max_size_t = (std::numeric_limits<size_t>::max)();
constexpr uint8_t byte_bits = 8;

// Consensus constants.
constexpr uint32_t reward_interval = 210000;
extern uint32_t coinbase_maturity;
constexpr uint32_t initial_block_reward = 50;
constexpr uint32_t max_work_bits = 0x1d00ffff;
constexpr uint32_t max_input_sequence = max_uint32;

constexpr uint32_t total_reward = 100000000;

// Threshold for nLockTime: below this value it is interpreted as block number,
// otherwise as UNIX timestamp. [Tue Nov 5 00:53:20 1985 UTC]
constexpr uint32_t locktime_threshold = 500000000;

constexpr uint64_t max_money_recursive(uint64_t current)
{
    return (current > 0) ? current + max_money_recursive(current >> 1) : 0;
}

constexpr uint64_t coin_price(uint64_t value=1)
{
    return value * 100000000;
}

constexpr uint64_t max_money()
{
    return coin_price(total_reward);
}

// For configuration settings initialization.
enum class settings
{
    none,
    mainnet,
    testnet
};

enum services: uint64_t
{
    // The node is capable of serving the block chain.
    node_network = (1 << 0),

    // Requires version >= 70004 (bip64)
    // The node is capable of responding to the getutxo protocol request.
    node_utxo = (1 << 1),

    // Requires version >= 70011 (proposed)
    // The node is capable and willing to handle bloom-filtered connections.
    bloom_filters = (1 << 2)
};

constexpr uint32_t no_timestamp = 0;
constexpr uint16_t unspecified_ip_port = 0;
constexpr message::ip_address unspecified_ip_address
{
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
    }
};
constexpr message::network_address unspecified_network_address
{
    no_timestamp,
    services::node_network,
    unspecified_ip_address,
    unspecified_ip_port
};

// TODO: make static.
BC_API hash_number max_target();

} // namespace libbitcoin

#endif
