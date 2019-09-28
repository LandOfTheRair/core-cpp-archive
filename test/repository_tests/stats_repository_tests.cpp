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
#include "repositories/stats_repository.h"
#include "repositories/users_repository.h"
#include "repositories/characters_repository.h"
#include "repositories/locations_repository.h"

using namespace std;
using namespace lotr;

TEST_CASE("stats repository tests") {
    stats_repository<database_pool, database_transaction> stat_repo(db_pool);
    users_repository<database_pool, database_transaction> users_repo(db_pool);
    characters_repository<database_pool, database_transaction> characters_repo(db_pool);
    locations_repository<database_pool, database_transaction> loc_repo(db_pool);

    SECTION( "stats inserted correctly" ) {
        auto transaction = stat_repo.create_transaction();
        db_location loc{0, "test", 0, 0};
        loc_repo.insert(loc, transaction);
        REQUIRE(loc.id > 0);
        user usr{0, "test", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);
        REQUIRE(usr.id > 0);
        db_character character{0, usr.id, loc.id, 0, 1, 2, "test", "allegiance", "gender", "alignment", "class", {}, {}, {}};
        characters_repo.insert(character, transaction);
        REQUIRE(character.id > 0);
        character_stat stat{0, character.id, "test", 2};
        stat_repo.insert(stat, transaction);
        REQUIRE(stat.id > 0);

        auto stat2 = stat_repo.get(stat.id, transaction);
        REQUIRE(stat2->id == stat.id);
        REQUIRE(stat2->character_id == stat.character_id);
        REQUIRE(stat2->name == stat.name);
        REQUIRE(stat2->value == stat.value);
    }

    SECTION( "update stats" ) {
        auto transaction = stat_repo.create_transaction();
        db_location loc{0, "test", 0, 0};
        loc_repo.insert(loc, transaction);
        REQUIRE(loc.id > 0);
        user usr{0, "test", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);
        REQUIRE(usr.id > 0);
        db_character character{0, usr.id, loc.id, 0, 1, 2, "test", "allegiance", "gender", "alignment", "class", {}, {}, {}};
        characters_repo.insert(character, transaction);
        REQUIRE(character.id > 0);
        character_stat stat{0, character.id, "test", 2};
        stat_repo.insert(stat, transaction);
        REQUIRE(stat.id > 0);

        stat.value = 12;
        stat_repo.update(stat, transaction);

        auto stat2 = stat_repo.get(stat.id, transaction);
        REQUIRE(stat2->id == stat.id);
        REQUIRE(stat2->character_id == stat.character_id);
        REQUIRE(stat2->name == stat.name);
        REQUIRE(stat2->value == stat.value);
    }

    SECTION( "get all for character stats" ) {
        auto transaction = stat_repo.create_transaction();
        db_location loc{0, "test", 0, 0};
        loc_repo.insert(loc, transaction);
        REQUIRE(loc.id > 0);
        user usr{0, "test", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);
        REQUIRE(usr.id > 0);
        db_character character{0, usr.id, loc.id, 0, 1, 2, "test", "allegiance", "gender", "alignment", "class", {}, {}, {}};
        characters_repo.insert(character, transaction);
        REQUIRE(character.id > 0);
        character_stat stat{0, character.id, "test", 20};
        stat_repo.insert(stat, transaction);
        REQUIRE(stat.id > 0);
        character_stat stat2{0, character.id, "test2", 30};
        stat_repo.insert(stat2, transaction);
        REQUIRE(stat2.id > 0);

        auto stats = stat_repo.get_by_character_id(character.id, transaction);
        REQUIRE(stats.size() == 2);
        REQUIRE(stats[0].id == stat.id);
        REQUIRE(stats[0].character_id == stat.character_id);
        REQUIRE(stats[0].name == stat.name);
        REQUIRE(stats[0].value == stat.value);
        REQUIRE(stats[1].id == stat2.id);
        REQUIRE(stats[1].character_id == stat2.character_id);
        REQUIRE(stats[1].name == stat2.name);
        REQUIRE(stats[1].value == stat2.value);
    }
}

#endif
