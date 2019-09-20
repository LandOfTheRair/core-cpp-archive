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

TEST_CASE("players repository tests") {
    users_repository<database_pool, database_transaction> users_repo(db_pool);
    characters_repository<database_pool, database_transaction> players_repo(db_pool);
    locations_repository<database_pool, database_transaction> locations_repo(db_pool);
    auto transaction = players_repo.create_transaction();


    SECTION( "db_character inserted correctly" ) {
        db_location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character plyr{0, usr.id, loc.id, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        auto inserted = players_repo.insert_or_update_player(plyr, transaction);
        REQUIRE(inserted == true);

        auto plyr2 = players_repo.get_player(plyr.id, included_tables::none, transaction);
        REQUIRE(plyr2);
        REQUIRE(plyr2->location_id == plyr.location_id);
        REQUIRE(plyr2->user_id == plyr.user_id);
        REQUIRE(plyr2->name == plyr.name);
        REQUIRE(plyr2->allegiance == plyr.allegiance);
        REQUIRE(plyr2->gender == plyr.gender);
        REQUIRE(plyr2->alignment == plyr.alignment);
        REQUIRE(plyr2->_class == plyr._class);
    }

    SECTION( "db_character updated correctly" ) {
        db_location loc{0, "test", 0, 0};
        db_location loc2{0, "test2", 0, 0};
        locations_repo.insert(loc, transaction);
        locations_repo.insert(loc2, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character plyr{0, usr.id, loc.id, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        auto inserted = players_repo.insert_or_update_player(plyr, transaction);
        REQUIRE(inserted == true);

        plyr.loc = loc2;
        plyr.allegiance = "allegiance2";
        plyr.gender = "gender2";
        plyr.alignment = "alignment2";
        plyr._class = "class2";
        inserted = players_repo.insert_or_update_player(plyr, transaction);
        REQUIRE(inserted == false);

        auto plyr2 = players_repo.get_player(plyr.id, included_tables::none, transaction);
        REQUIRE(plyr2);
        REQUIRE(plyr2->location_id == plyr.location_id);
        REQUIRE(plyr2->user_id == plyr.user_id);
        REQUIRE(plyr2->name == plyr.name);
        REQUIRE(plyr2->allegiance == plyr.allegiance);
        REQUIRE(plyr2->gender == plyr.gender);
        REQUIRE(plyr2->alignment == plyr.alignment);
        REQUIRE(plyr2->_class == plyr._class);
    }

    SECTION( "Can't insert db_character twice" ) {
        db_location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character plyr{0, usr.id, loc.id, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        auto ret = players_repo.insert(plyr, transaction);
        REQUIRE(ret == true);

        ret = players_repo.insert(plyr, transaction);
        REQUIRE(ret == false);

        auto plyr2 = players_repo.get_player(plyr.id, included_tables::none, transaction);
        REQUIRE(plyr2);
        REQUIRE(plyr2->location_id == plyr.location_id);
        REQUIRE(plyr2->user_id == plyr.user_id);
        REQUIRE(plyr2->name == plyr.name);
        REQUIRE(plyr2->allegiance == plyr.allegiance);
        REQUIRE(plyr2->gender == plyr.gender);
        REQUIRE(plyr2->alignment == plyr.alignment);
        REQUIRE(plyr2->_class == plyr._class);
    }

    SECTION( "multiple players retrieved correctly" ) {
        db_location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        db_character plyr{0, usr.id, loc.id, "john doe"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        db_character plyr2{0, usr.id, loc.id, "john doe2"s, "allegiance", "gender", "alignment", "class", {}, {}, {}};
        players_repo.insert_or_update_player(plyr, transaction);
        players_repo.insert_or_update_player(plyr2, transaction);

        auto players = players_repo.get_by_user_id(plyr.user_id, included_tables::none, transaction);
        REQUIRE(players.size() == 2);
        REQUIRE(!players[0].loc);
        REQUIRE(players[0].items.empty());
        REQUIRE(players[0].stats.empty());

        players = players_repo.get_by_user_id(plyr.user_id, included_tables::location, transaction);
        REQUIRE(players.size() == 2);
        REQUIRE(players[0].loc);
        REQUIRE(players[0].loc->map_name == "test");
        REQUIRE(players[0].loc->x == loc.x);
        REQUIRE(players[0].loc->y == loc.y);
        REQUIRE(players[0].items.empty());
        REQUIRE(players[0].stats.empty());
    }
}

#endif
