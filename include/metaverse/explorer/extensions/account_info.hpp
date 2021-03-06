/**
 * Copyright (c) 2011-2015 metaverse developers (see AUTHORS)
 *
 * This file is part of mvs-node.
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
#pragma once

#include <cstdint>
#include <istream>
#include <vector>
#include <metaverse/bitcoin/chain/point.hpp>
#include <metaverse/bitcoin/chain/script/script.hpp>
#include <metaverse/bitcoin/define.hpp>
#include <metaverse/bitcoin/utility/reader.hpp>
#include <metaverse/bitcoin/utility/writer.hpp>
#include <metaverse/bitcoin/formats/base_16.hpp>
#include <metaverse/bitcoin/chain/attachment/account/account.hpp>
#include <metaverse/bitcoin/chain/attachment/account/account_address.hpp>
#include <metaverse/bitcoin/chain/attachment/asset/asset_detail.hpp>

namespace libbitcoin {
namespace chain {
	using StoreAccountFunc = std::function<void(const std::shared_ptr<account>&)>;
	using StoreAddressFunc = std::function<void(const std::shared_ptr<account_address>&)>;
	using StoreAssetFunc = 
		std::function<void(const std::shared_ptr<asset_detail>&, const std::string&)>;
// used for store all account related information
class BC_API account_info
{
public:
	account_info(std::string& passphrase);
	account_info(std::string& passphrase,
        account& meta, std::vector<account_address>& addr_vec, std::vector<asset_detail>& asset_vec);

    bool from_data(const data_chunk& data);
    bool from_data(std::istream& stream);
    bool from_data(reader& source);
    data_chunk to_data() const;
    void to_data(std::ostream& stream) const;
    void to_data(writer& sink) const;
    void store(std::string& name, std::string& passwd, 
		StoreAccountFunc store_account, StoreAddressFunc store_account_address,
		StoreAssetFunc store_account_asset);
    account get_account() const;
    std::vector<account_address>& get_account_address(); 
    std::vector<asset_detail>& get_account_asset();
    void encrypt();
    void decrypt(std::string& hexcode);
    friend std::istream& operator>>(std::istream& input, account_info& self_ref);
    friend std::ostream& operator<<(std::ostream& output, const account_info& self_ref);

private:
	account meta_;
	std::vector<account_address> addr_vec_;
	std::vector<asset_detail> asset_vec_;
    // encrypt/decrypt
    data_chunk data_;
    std::string passphrase_;
};

} // namespace chain
} // namespace libbitcoin



