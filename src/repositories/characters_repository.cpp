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

#include "characters_repository.h"
#include <spdlog/spdlog.h>

using namespace lotr;

template class lotr::characters_repository<database_pool, database_transaction>;

template<typename pool_T, typename transaction_T>
characters_repository<pool_T, transaction_T>::characters_repository(shared_ptr<pool_T> database_pool) : _database_pool(move(database_pool)) {

}

template<typename pool_T, typename transaction_T>
unique_ptr<transaction_T> characters_repository<pool_T, transaction_T>::create_transaction() {
    return _database_pool->create_transaction();
}

template<typename pool_T, typename transaction_T>
bool characters_repository<pool_T, transaction_T>::insert(db_character &character, unique_ptr<transaction_T> const &transaction) const {

    auto result = transaction->execute(fmt::format(
            "INSERT INTO characters (user_id, location_id, slot, level, gold, character_name, allegiance, gender, alignment, class) VALUES ({}, {}, {}, {}, {}, '{}', '{}', '{}', '{}', '{}') "
            "ON CONFLICT (user_id, slot) DO NOTHING RETURNING xmax, id",
            character.user_id, character.location_id, character.slot, character.level, character.gold, transaction->escape(character.name), transaction->escape(character.allegiance),
            transaction->escape(character.gender), transaction->escape(character.alignment), transaction->escape(character._class)));

    if(result.empty()) {
        spdlog::error("[{}] contains {} entries", __FUNCTION__, result.size());
        return false;
    }

    character.id = result[0][1].as(uint64_t{});

    if(result[0][0].as(uint64_t{}) == 0) {
        spdlog::debug("[{}] inserted db_character {}", __FUNCTION__, character.id);
        return true;
    }

    spdlog::debug("[{}] could not insert db_character {} {}", __FUNCTION__, character.id, character.name);
    return false;
}

template<typename pool_T, typename transaction_T>
bool characters_repository<pool_T, transaction_T>::insert_or_update_character(db_character &character, unique_ptr<transaction_T> const &transaction) const {

    auto result = transaction->execute(fmt::format(
            "INSERT INTO characters (user_id, location_id, slot, level, gold, character_name, allegiance, gender, alignment, class) VALUES ({}, {}, {}, {}, {}, '{}', '{}', '{}', '{}', '{}') "
            "ON CONFLICT (user_id, slot) DO UPDATE SET user_id = {}, location_id = {}, level = {}, gold = {}, allegiance = '{}', gender = '{}', alignment = '{}', class = '{}' RETURNING xmax, id",
            character.user_id, character.location_id, character.slot, character.level, character.gold, transaction->escape(character.name), transaction->escape(character.allegiance),
            transaction->escape(character.gender), transaction->escape(character.alignment), transaction->escape(character._class),
            character.user_id, character.location_id, character.level, character.gold,
            transaction->escape(character.allegiance), transaction->escape(character.gender), transaction->escape(character.alignment), transaction->escape(character._class)));

    if(result.empty()) {
        spdlog::error("[{}] contains {} entries", __FUNCTION__, result.size());
        return false;
    }

    character.id = result[0][1].as(uint64_t{});

    if(result[0][0].as(uint64_t{}) == 0) {
        spdlog::debug("[{}] inserted db_character {}", __FUNCTION__, character.id);
        return true;
    }

    spdlog::debug("[{}] updated db_character {}", __FUNCTION__, character.id);
    return false;
}

template<typename pool_T, typename transaction_T>
void characters_repository<pool_T, transaction_T>::update_character(db_character const &character, unique_ptr<transaction_T> const &transaction) const {
    transaction->execute(fmt::format("UPDATE characters SET user_id = {}, location_id = {}, level = {}, gold = {}, allegiance = '{}', gender = '{}', alignment = '{}', class = '{}' WHERE id = {}",
            character.user_id, character.location_id, character.level, character.gold, character.allegiance, character.gender, character.alignment, character._class, character.id));

    spdlog::debug("[{}] updated db_character {}", __FUNCTION__, character.id);
}

template<typename pool_T, typename transaction_T>
void characters_repository<pool_T, transaction_T>::delete_character_by_slot(uint32_t slot, uint64_t user_id, unique_ptr<transaction_T> const &transaction) const {
    transaction->execute(fmt::format("DELETE FROM characters WHERE slot = {} AND user_id = {}", slot, user_id));

    spdlog::debug("[{}] deleted db_character {} for user {}", __FUNCTION__, slot, user_id);
}

template<typename pool_T, typename transaction_T>
optional<db_character> characters_repository<pool_T, transaction_T>::get_character(string const &name, uint64_t user_id, included_tables includes,
                                                                                   unique_ptr<transaction_T> const &transaction) const {
    pqxx::result result;

    if(includes == included_tables::none) {
        result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.slot, p.level, p.gold, p.character_name, p.allegiance, p.gender, p.alignment, p.class FROM characters p WHERE character_name = '{}' and p.user_id = {}", transaction->escape(name), user_id));
    } else if (includes == included_tables::location) {
        result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.slot, p.level, p.gold, p.character_name, p.allegiance, p.gender, p.alignment, p.class, l.id, l.map_name, l.x, l.y FROM characters p "
                                              "INNER JOIN locations l ON l.id = p.location_id "
                                              "WHERE p.character_name = '{}' and p.user_id = {}", transaction->escape(name), user_id));
    } else {
        spdlog::debug("[{}] included_tables value {} not implemented", __FUNCTION__, static_cast<int>(includes));
        return {};
    }

    if(result.empty()) {
        spdlog::debug("[{}] found no db_character by name {}", __FUNCTION__, name);
        return {};
    }

    auto ret = make_optional<db_character>(result[0][0].as(uint64_t{}), result[0][1].as(uint64_t{}),
                                           result[0][2].as(uint64_t{}), result[0][3].as(uint32_t{}),
                                           result[0][4].as(uint32_t{}), result[0][5].as(uint32_t{}),
                                           result[0][6].as(string{}), result[0][7].as(string{}),
                                           result[0][8].as(string{}), result[0][9].as(string{}),
                                           result[0][10].as(string{}),
                                           optional<db_location>{}, vector<character_stat>{}, vector<character_item>{});

    if(includes == included_tables::location) {
        ret->loc.emplace(result[0][11].as(uint64_t{}), result[0][12].as(string{}), result[0][13].as(uint32_t{}), result[0][14].as(uint32_t{}));
    }

    spdlog::debug("[{}] found db_character by name {} with id {}", __FUNCTION__, name, ret->id);

    return ret;
}

template<typename pool_T, typename transaction_T>
optional<db_character> characters_repository<pool_T, transaction_T>::get_character(uint64_t id, uint64_t user_id, included_tables includes,
                                                                                   unique_ptr<transaction_T> const &transaction) const {
    auto result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.slot, p.level, p.gold, p.character_name, p.allegiance, p.gender, p.alignment, p.class FROM characters p WHERE id = {} and user_id = {}", id, user_id));

    if(result.empty()) {
        spdlog::debug("[{}] found no db_character by id {}", __FUNCTION__, id);
        return {};
    }

    auto ret = make_optional<db_character>(result[0][0].as(uint64_t{}), result[0][1].as(uint64_t{}),
                                           result[0][2].as(uint64_t{}), result[0][3].as(uint32_t{}),
                                           result[0][4].as(uint32_t{}), result[0][5].as(uint32_t{}),
                                           result[0][6].as(string{}), result[0][7].as(string{}),
                                           result[0][8].as(string{}), result[0][9].as(string{}),
                                           result[0][10].as(string{}),
                                           optional<db_location>{}, vector<character_stat>{}, vector<character_item>{});

    spdlog::debug("[{}] found db_character by id {}", __FUNCTION__, id);

    return ret;
}

template<typename pool_T, typename transaction_T>
optional<db_character> characters_repository<pool_T, transaction_T>::get_character_by_slot(uint32_t slot, uint64_t user_id, included_tables includes,
                                                                                   unique_ptr<transaction_T> const &transaction) const {
    pqxx::result result;

    if(includes == included_tables::none) {
        result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.slot, p.level, p.gold, p.character_name, p.allegiance, p.gender, p.alignment, p.class FROM characters p WHERE slot = {} and user_id = {}", slot, user_id));
    } else if (includes == included_tables::location) {
        result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.slot, p.level, p.gold, p.character_name, p.allegiance, p.gender, p.alignment, p.class, l.id, l.map_name, l.x, l.y FROM characters p "
                                                  "INNER JOIN locations l ON l.id = p.location_id "
                                                  "WHERE slot = {} and user_id = {}", slot, user_id));
    } else {
        spdlog::debug("[{}] included_tables value {} not implemented", __FUNCTION__, static_cast<int>(includes));
        return {};
    }

    if(result.empty()) {
        spdlog::debug("[{}] found no db_character by slot {}", __FUNCTION__, slot);
        return {};
    }

    auto ret = make_optional<db_character>(result[0][0].as(uint64_t{}), result[0][1].as(uint64_t{}),
                                           result[0][2].as(uint64_t{}), result[0][3].as(uint32_t{}),
                                           result[0][4].as(uint32_t{}), result[0][5].as(uint32_t{}),
                                           result[0][6].as(string{}), result[0][7].as(string{}),
                                           result[0][8].as(string{}), result[0][9].as(string{}),
                                           result[0][10].as(string{}),
                                           optional<db_location>{}, vector<character_stat>{}, vector<character_item>{});

    if(includes == included_tables::location) {
        ret->loc.emplace(result[0][11].as(uint64_t{}), result[0][12].as(string{}), result[0][13].as(uint32_t{}), result[0][14].as(uint32_t{}));
    }

    spdlog::debug("[{}] found db_character by slot {} for user {}", __FUNCTION__, slot, user_id);

    return ret;
}

template<typename pool_T, typename transaction_T>
vector<db_character> characters_repository<pool_T, transaction_T>::get_by_user_id(uint64_t user_id, included_tables includes,
                                                                                  unique_ptr<transaction_T> const &transaction) const {
    pqxx::result result;
    if(includes == included_tables::none) {
        result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.slot, p.level, p.gold, p.character_name, p.allegiance, p.gender, p.alignment, p.class FROM characters p WHERE user_id = {}", user_id));
    } else if (includes == included_tables::location) {
        result = transaction->execute(fmt::format("SELECT p.id, p.user_id, p.location_id, p.slot, p.level, p.gold, p.character_name, p.allegiance, p.gender, p.alignment, p.class, l.id, l.map_name, l.x, l.y FROM characters p "
                                      "INNER JOIN locations l ON l.id = p.location_id "
                                      "WHERE p.user_id = {}", user_id));
    } else {
        spdlog::debug("[{}] included_tables value {} not implemented", __FUNCTION__, static_cast<int>(includes));
        return {};
    }

    spdlog::debug("[{}] contains {} entries", __FUNCTION__, result.size());

    vector<db_character> characters;
    characters.reserve(result.size());

    for(auto const & res : result) {
        db_character character{res[0].as(uint64_t{}), res[1].as(uint64_t{}), res[2].as(uint64_t{}), res[3].as(uint32_t{}), res[4].as(uint32_t{}),
                               res[5].as(uint32_t{}), res[6].as(string{}), res[7].as(string{}), res[8].as(string{}), res[9].as(string{}), res[10].as(string{}), {}, {}, {}};
        if(includes == included_tables::location) {
            character.loc.emplace(result[0][11].as(uint64_t{}), result[0][12].as(string{}), result[0][13].as(uint32_t{}), result[0][14].as(uint32_t{}));
        }
        characters.push_back(move(character));
    }

    return characters;
}
