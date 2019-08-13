/*
    RealmOfAesirWorld
    Copyright (C) 2017  Michael de Lang

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "locations_repository.h"
#include <spdlog/spdlog.h>

using namespace lotr;

template class lotr::locations_repository<database_pool, database_transaction>;

template<typename pool_T, typename transaction_T>
locations_repository<pool_T, transaction_T>::locations_repository(shared_ptr<pool_T> database_pool) : _database_pool(move(database_pool)) {

}

template<typename pool_T, typename transaction_T>
unique_ptr<transaction_T> locations_repository<pool_T, transaction_T>::create_transaction() {
    return _database_pool->create_transaction();
}

template<typename pool_T, typename transaction_T>
void locations_repository<pool_T, transaction_T>::insert(location &loc, unique_ptr<transaction_T> const &transaction) const {
    auto result = transaction->execute(fmt::format("INSERT INTO locations (map_name, x, y) VALUES ('{}', {}, {}) RETURNING id", loc.map_name, loc.x, loc.y));

    if(result.empty()) {
        spdlog::error("{} contains {} entries", __FUNCTION__, result.size());
        return;
    }

    loc.id = result[0][0].as(uint32_t{});

    spdlog::debug("{} inserted location {}", __FUNCTION__, loc.id);
}

template<typename pool_T, typename transaction_T>
void locations_repository<pool_T, transaction_T>::update(location const &loc, unique_ptr<transaction_T> const &transaction) const {
    transaction->execute(fmt::format("UPDATE locations SET map_name = {}, x = {}, y = {} WHERE id = {}", loc.map_name, loc.x, loc.y, loc.id));

    spdlog::debug("{} updated location {}", __FUNCTION__, loc.id);
}

template<typename pool_T, typename transaction_T>
optional<location> locations_repository<pool_T, transaction_T>::get(uint64_t id, unique_ptr<transaction_T> const &transaction) const {
    auto result = transaction->execute(fmt::format("SELECT l.id, l.map_name, l.x, l.y FROM locations l WHERE id = {}" , id));

    if(result.empty()) {
        spdlog::error("{} found no location by id {}", __FUNCTION__, id);
        return {};
    }

    auto ret = make_optional<location>(result[0][0].as(uint64_t{}), result[0][1].as(string{}),
                                       result[0][2].as(uint32_t{}), result[0][3].as(uint32_t{}));

    spdlog::error("{} found location by id {}", __FUNCTION__, id);

    return ret;
}

template<typename pool_T, typename transaction_T>
vector<location> locations_repository<pool_T, transaction_T>::get_by_map_name(string map_name, unique_ptr<transaction_T> const &transaction) const {
    auto result = transaction->execute(fmt::format("SELECT l.id, l.map_name, l.x, l.y FROM locations l WHERE map_name = '{}'", transaction->escape(map_name)));

    spdlog::debug("{} contains {} entries", __FUNCTION__, result.size());

    vector<location> locations;
    locations.reserve(result.size());

    for(auto const & res : result) {
        locations.emplace_back(result[0][0].as(uint64_t{}), result[0][1].as(string{}),
                               result[0][2].as(uint32_t{}), result[0][3].as(uint32_t{}));
    }

    return locations;
}
