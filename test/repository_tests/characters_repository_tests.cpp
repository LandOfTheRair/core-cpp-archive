/*
    Land of the Rair
    Copyright (C) 2019  Michael de Lang

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

#ifndef EXCLUDE_PSQL_TESTS

#include <catch2/catch.hpp>
#include "../test_helpers/startup_helper.h"
#include "repositories/users_repository.h"
#include "repositories/characters_repository.h"
#include "repositories/locations_repository.h"

using namespace std;
using namespace lotr;

TEST_CASE("characters repository tests") {
    users_repository<database_pool, database_transaction> users_repo(db_pool);
    characters_repository<database_pool, database_transaction> characters_repo(db_pool);
    locations_repository<database_pool, database_transaction> locations_repo(db_pool);
    auto transaction = characters_repo.create_transaction();


    SECTION( "db_character inserted correctly" ) {
        db_location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character character{0, usr.id, loc.id, 1, 2, 3, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        auto inserted = characters_repo.insert_or_update_character(character, transaction);
        REQUIRE(inserted == true);

        auto character2 = characters_repo.get_character(character.id, usr.id, included_tables::none, transaction);
        REQUIRE(character2);
        REQUIRE(character2->location_id == character.location_id);
        REQUIRE(character2->user_id == character.user_id);
        REQUIRE(character2->slot == character.slot);
        REQUIRE(character2->level == character.level);
        REQUIRE(character2->gold == character.gold);
        REQUIRE(character2->name == character.name);
        REQUIRE(character2->allegiance == character.allegiance);
        REQUIRE(character2->gender == character.gender);
        REQUIRE(character2->alignment == character.alignment);
        REQUIRE(character2->_class == character._class);
    }

    SECTION( "db_character only for user" ) {
        db_location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character character{0, usr.id, loc.id, 1, 2, 3, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        auto inserted = characters_repo.insert_or_update_character(character, transaction);
        REQUIRE(inserted == true);

        auto character2 = characters_repo.get_character(character.id, usr.id + 1, included_tables::none, transaction);
        REQUIRE(!character2);
    }

    SECTION( "db_character updated correctly" ) {
        db_location loc{0, "test", 0, 0};
        db_location loc2{0, "test2", 0, 0};
        locations_repo.insert(loc, transaction);
        locations_repo.insert(loc2, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character character{0, usr.id, loc.id, 1, 2, 3, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        auto inserted = characters_repo.insert_or_update_character(character, transaction);
        REQUIRE(inserted == true);

        character.loc = loc2;
        character.allegiance = "allegiance2";
        character.gender = "gender2";
        character.alignment = "alignment2";
        character._class = "class2";
        character.level = 3;
        character.gold = 4;
        inserted = characters_repo.insert_or_update_character(character, transaction);
        REQUIRE(inserted == false);

        auto character2 = characters_repo.get_character(character.id, usr.id, included_tables::none, transaction);
        REQUIRE(character2);
        REQUIRE(character2->location_id == character.location_id);
        REQUIRE(character2->user_id == character.user_id);
        REQUIRE(character2->name == character.name);
        REQUIRE(character2->level == character.level);
        REQUIRE(character2->gold == character.gold);
        REQUIRE(character2->allegiance == character.allegiance);
        REQUIRE(character2->gender == character.gender);
        REQUIRE(character2->alignment == character.alignment);
        REQUIRE(character2->_class == character._class);
    }
    
    SECTION( "db_character get by name with location working correctly" ) {
        db_location loc{0, "test", 0, 0};
        db_location loc2{0, "test2", 0, 0};
        locations_repo.insert(loc, transaction);
        locations_repo.insert(loc2, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character character{0, usr.id, loc.id, 1, 2, 3, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        auto inserted = characters_repo.insert_or_update_character(character, transaction);
        REQUIRE(inserted == true);

        character.loc = loc2;
        character.allegiance = "allegiance2";
        character.gender = "gender2";
        character.alignment = "alignment2";
        character._class = "class2";
        character.level = 3;
        character.gold = 4;
        inserted = characters_repo.insert_or_update_character(character, transaction);
        REQUIRE(inserted == false);

        auto character2 = characters_repo.get_character(character.name, usr.id, included_tables::none, transaction);
        REQUIRE(character2);
        REQUIRE(character2->location_id == character.location_id);
        REQUIRE(character2->user_id == character.user_id);
        REQUIRE(character2->name == character.name);
        REQUIRE(character2->level == character.level);
        REQUIRE(character2->gold == character.gold);
        REQUIRE(character2->allegiance == character.allegiance);
        REQUIRE(character2->gender == character.gender);
        REQUIRE(character2->alignment == character.alignment);
        REQUIRE(character2->_class == character._class);
    }

    SECTION( "Can't insert db_character twice" ) {
        db_location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character character{0, usr.id, loc.id, 1, 2, 3, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        auto ret = characters_repo.insert(character, transaction);
        REQUIRE(ret == true);

        ret = characters_repo.insert(character, transaction);
        REQUIRE(ret == false);

        auto character2 = characters_repo.get_character(character.id, usr.id, included_tables::none, transaction);
        REQUIRE(character2);
        REQUIRE(character2->location_id == character.location_id);
        REQUIRE(character2->user_id == character.user_id);
        REQUIRE(character2->slot == character.slot);
        REQUIRE(character2->level == character.level);
        REQUIRE(character2->gold == character.gold);
        REQUIRE(character2->name == character.name);
        REQUIRE(character2->allegiance == character.allegiance);
        REQUIRE(character2->gender == character.gender);
        REQUIRE(character2->alignment == character.alignment);
        REQUIRE(character2->_class == character._class);
    }

    SECTION( "multiple characters retrieved correctly" ) {
        db_location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character character{0, usr.id, loc.id, 0, 2, 4, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        db_character character2{0, usr.id, loc.id, 1, 3, 5, "john doe2"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        characters_repo.insert_or_update_character(character, transaction);
        characters_repo.insert_or_update_character(character2, transaction);

        auto characters = characters_repo.get_by_user_id(character.user_id, included_tables::none, transaction);
        REQUIRE(characters.size() == 2);
        REQUIRE(!characters[0].loc);
        REQUIRE(characters[0].items.empty());
        REQUIRE(characters[0].stats.empty());

        characters = characters_repo.get_by_user_id(character.user_id, included_tables::location, transaction);
        REQUIRE(characters.size() == 2);
        REQUIRE(characters[0].loc);
        REQUIRE(characters[0].loc->map_name == "test");
        REQUIRE(characters[0].loc->x == loc.x);
        REQUIRE(characters[0].loc->y == loc.y);
        REQUIRE(characters[0].items.empty());
        REQUIRE(characters[0].stats.empty());
    }

    SECTION( "Get character by slot" ) {
        db_location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character character{0, usr.id, loc.id, 0, 2, 4, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        db_character character2{0, usr.id, loc.id, 1, 3, 5, "john doe2"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        characters_repo.insert_or_update_character(character, transaction);
        characters_repo.insert_or_update_character(character2, transaction);

        auto character_by_slot = characters_repo.get_character_by_slot(0, usr.id, included_tables::none, transaction);
        REQUIRE(character_by_slot->location_id == character.location_id);
        REQUIRE(character_by_slot->user_id == character.user_id);
        REQUIRE(character_by_slot->slot == character.slot);
        REQUIRE(character_by_slot->level == character.level);
        REQUIRE(character_by_slot->gold == character.gold);
        REQUIRE(character_by_slot->name == character.name);
        REQUIRE(character_by_slot->allegiance == character.allegiance);
        REQUIRE(character_by_slot->gender == character.gender);
        REQUIRE(character_by_slot->alignment == character.alignment);
        REQUIRE(character_by_slot->_class == character._class);

        character_by_slot = characters_repo.get_character_by_slot(1, usr.id, included_tables::none, transaction);
        REQUIRE(character_by_slot->location_id == character2.location_id);
        REQUIRE(character_by_slot->user_id == character2.user_id);
        REQUIRE(character_by_slot->slot == character2.slot);
        REQUIRE(character_by_slot->level == character2.level);
        REQUIRE(character_by_slot->gold == character2.gold);
        REQUIRE(character_by_slot->name == character2.name);
        REQUIRE(character_by_slot->allegiance == character2.allegiance);
        REQUIRE(character_by_slot->gender == character2.gender);
        REQUIRE(character_by_slot->alignment == character2.alignment);
        REQUIRE(character_by_slot->_class == character2._class);
    }

    SECTION( "Get character by slot with location" ) {
        db_location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character character{0, usr.id, loc.id, 0, 2, 4, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        db_character character2{0, usr.id, loc.id, 1, 3, 5, "john doe2"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        characters_repo.insert_or_update_character(character, transaction);
        characters_repo.insert_or_update_character(character2, transaction);

        auto character_by_slot = characters_repo.get_character_by_slot(0, usr.id, included_tables::location, transaction);
        REQUIRE(character_by_slot->location_id == character.location_id);
        REQUIRE(character_by_slot->user_id == character.user_id);
        REQUIRE(character_by_slot->slot == character.slot);
        REQUIRE(character_by_slot->level == character.level);
        REQUIRE(character_by_slot->gold == character.gold);
        REQUIRE(character_by_slot->name == character.name);
        REQUIRE(character_by_slot->allegiance == character.allegiance);
        REQUIRE(character_by_slot->gender == character.gender);
        REQUIRE(character_by_slot->alignment == character.alignment);
        REQUIRE(character_by_slot->_class == character._class);
        REQUIRE(character_by_slot->loc);
        REQUIRE(character_by_slot->loc->id == loc.id);
        REQUIRE(character_by_slot->loc->map_name == loc.map_name);
        REQUIRE(character_by_slot->loc->x == loc.x);
        REQUIRE(character_by_slot->loc->y == loc.y);

        character_by_slot = characters_repo.get_character_by_slot(1, usr.id, included_tables::location, transaction);
        REQUIRE(character_by_slot->location_id == character2.location_id);
        REQUIRE(character_by_slot->user_id == character2.user_id);
        REQUIRE(character_by_slot->slot == character2.slot);
        REQUIRE(character_by_slot->level == character2.level);
        REQUIRE(character_by_slot->gold == character2.gold);
        REQUIRE(character_by_slot->name == character2.name);
        REQUIRE(character_by_slot->allegiance == character2.allegiance);
        REQUIRE(character_by_slot->gender == character2.gender);
        REQUIRE(character_by_slot->alignment == character2.alignment);
        REQUIRE(character_by_slot->_class == character2._class);
        REQUIRE(character_by_slot->loc);
        REQUIRE(character_by_slot->loc->id == loc.id);
        REQUIRE(character_by_slot->loc->map_name == loc.map_name);
        REQUIRE(character_by_slot->loc->x == loc.x);
        REQUIRE(character_by_slot->loc->y == loc.y);
    }

    SECTION( "Delete character by slot" ) {
        db_location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character character{0, usr.id, loc.id, 0, 2, 4, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        db_character character2{0, usr.id, loc.id, 1, 3, 5, "john doe2"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        characters_repo.insert_or_update_character(character, transaction);
        characters_repo.insert_or_update_character(character2, transaction);
        characters_repo.delete_character_by_slot(0, usr.id, transaction);

        auto character_by_slot = characters_repo.get_character_by_slot(0, usr.id, included_tables::none, transaction);
        REQUIRE(!character_by_slot);

        character_by_slot = characters_repo.get_character_by_slot(1, usr.id, included_tables::none, transaction);
        REQUIRE(character_by_slot->location_id == character2.location_id);
        REQUIRE(character_by_slot->user_id == character2.user_id);
        REQUIRE(character_by_slot->slot == character2.slot);
        REQUIRE(character_by_slot->level == character2.level);
        REQUIRE(character_by_slot->gold == character2.gold);
        REQUIRE(character_by_slot->name == character2.name);
        REQUIRE(character_by_slot->allegiance == character2.allegiance);
        REQUIRE(character_by_slot->gender == character2.gender);
        REQUIRE(character_by_slot->alignment == character2.alignment);
        REQUIRE(character_by_slot->_class == character2._class);
    }
}

#endif
