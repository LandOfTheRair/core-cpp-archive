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

#include "players_repository.h"
#include <spdlog/spdlog.h>

using namespace lotr;

template class lotr::players_repository<database_pool, database_transaction>;

template<typename pool_T, typename transaction_T>
players_repository<pool_T, transaction_T>::players_repository(shared_ptr<pool_T> database_pool) : _database_pool(move(database_pool)) {

}

template<typename pool_T, typename transaction_T>
unique_ptr<transaction_T> players_repository<pool_T, transaction_T>::create_transaction() {
    return _database_pool->create_transaction();
}

template<typename pool_T, typename transaction_T>
bool players_repository<pool_T, transaction_T>::insert_or_update_player(player &plyr, unique_ptr<transaction_T> const &transaction) {

    auto result = transaction->execute(fmt::format(
            "INSERT INTO players (user_id, location_id, player_name) VALUES ({}, {}, '{}') "
            "ON CONFLICT (player_name) DO UPDATE SET user_id = {}, location_id = {} RETURNING xmax, id",
            plyr.user_id, plyr.location_id, transaction->escape(plyr.name), plyr.user_id, plyr.location_id));

    if(result.empty()) {
        spdlog::error("{} contains {} entries", __FUNCTION__, result.size());
        return false;
    }

    plyr.id = result[0][1].as(uint64_t{});

    if(result[0][0].as(uint64_t{}) == 0) {
        spdlog::debug("{} inserted player {}", __FUNCTION__, plyr.id);
        return true;
    }

    spdlog::debug("{} updated player {}", __FUNCTION__, plyr.id);
    return false;
}

template<typename pool_T, typename transaction_T>
void players_repository<pool_T, transaction_T>::update_player(player const &plyr, unique_ptr<transaction_T> const &transaction) {
    transaction->execute(fmt::format("UPDATE players SET user_id = {}, location_id = {} WHERE id = {}", plyr.user_id, plyr.location_id, plyr.id));

    spdlog::debug("{} updated player {}", __FUNCTION__, plyr.id);
}

template<typename pool_T, typename transaction_T>
optional<player> players_repository<pool_T, transaction_T>::get_player(string const &name, included_tables includes,
                                                    unique_ptr<transaction_T> const &transaction) {
    pqxx::result result;

    if(includes == included_tables::none) {
        result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.player_name FROM players p WHERE player_name = '{}'", transaction->escape(name)));
    } else if (includes == included_tables::location) {
        result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.player_name, l.id, l.map_name, l.x, l.y FROM players p "
                                              "INNER JOIN locations l ON l.id = p.location_id "
                                              "WHERE p.player_name = '{}'", transaction->escape(name)));
    } else {
        spdlog::debug("{} included_tables value {} not implemented", __FUNCTION__, static_cast<int>(includes));
        return {};
    }

    if(result.empty()) {
        spdlog::debug("{} found no player by name {}", __FUNCTION__, name);
        return {};
    }

    auto ret = make_optional<player>(result[0][0].as(uint64_t{}), result[0][1].as(uint64_t{}),
                                     result[0][2].as(uint64_t{}), result[0][3].as(string{}), {}, {}, {});

    if(includes == included_tables::location) {
        ret->loc.emplace(result[0][4].as(uint64_t{}), result[0][5].as(string{}), result[0][6].as(uint32_t{}), result[0][7].as(uint32_t{}));
    }

    spdlog::debug("{} found player by name {} with id {}", __FUNCTION__, name, ret->id);

    return ret;
}

template<typename pool_T, typename transaction_T>
optional<player> players_repository<pool_T, transaction_T>::get_player(uint64_t id, included_tables includes,
                                                    unique_ptr<transaction_T> const &transaction) {
    auto result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.player_name FROM players p WHERE id = {}", id));

    if(result.empty()) {
        spdlog::debug("{} found no player by id {}", __FUNCTION__, id);
        return {};
    }

    auto ret = make_optional<player>(result[0][0].as(uint64_t{}), result[0][1].as(uint64_t{}),
                                      result[0][2].as(uint64_t{}), result[0][3].as(string{}));

    spdlog::debug("{} found player by id {}", __FUNCTION__, id);

    return ret;
}

template<typename pool_T, typename transaction_T>
vector<player> players_repository<pool_T, transaction_T>::get_by_user_id(uint64_t user_id, included_tables includes,
                                                          unique_ptr<transaction_T> const &transaction) {
    pqxx::result result;
    if(includes == included_tables::none) {
        result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.player_name FROM players p WHERE user_id = {}", user_id));
    } else if (includes == included_tables::location) {
        result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.player_name, l.id, l.map_name, l.x, l.y FROM players p "
                                      "INNER JOIN locations l ON l.id = p.location_id "
                                      "WHERE p.user_id = {}", user_id));
    } else {
        spdlog::debug("{} included_tables value {} not implemented", __FUNCTION__, static_cast<int>(includes));
        return {};
    }

    spdlog::debug("{} contains {} entries", __FUNCTION__, result.size());

    vector<player> players;
    players.reserve(result.size());

    for(auto const & res : result) {
        player plyr{res[0].as(uint64_t{}), res[1].as(uint64_t{}), res[2].as(uint64_t{}), res[3].as(string{}), {}, {}, {}};
        if(includes == included_tables::location) {
            plyr.loc.emplace(result[0][4].as(uint64_t{}), result[0][5].as(string{}), result[0][6].as(uint32_t{}), result[0][7].as(uint32_t{}));
        }
        players.push_back(move(plyr));
    }

    return players;
}
