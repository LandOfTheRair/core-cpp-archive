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

#include <catch2/catch.hpp>
#include "test_helpers/startup_helper.h"
#include "../src/repositories/users_repository.h"
#include "../src/repositories/players_repository.h"
#include "../src/repositories/locations_repository.h"

using namespace std;
using namespace lotr;

TEST_CASE("players repository tests") {
    auto pool = make_shared<database_pool>();
    pool->create_connections(config.connection_string, 1);

    users_repository<database_pool, database_transaction> users_repo(pool);
    players_repository<database_pool, database_transaction> players_repo(pool);
    locations_repository<database_pool, database_transaction> locations_repo(pool);
    auto transaction = players_repo.create_transaction();


    SECTION( "player inserted correctly" ) {
        location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        player plyr{0, usr.id, loc.id, "john doe"s, {}, {}, {}};
        players_repo.insert_or_update_player(plyr, transaction);

        auto plyr2 = players_repo.get_player(plyr.id, included_tables::none, transaction);
        REQUIRE(plyr2);
        REQUIRE(plyr2->location_id == plyr.location_id);
        REQUIRE(plyr2->user_id == plyr.user_id);
        REQUIRE(plyr2->name == plyr.name);
    }

    SECTION( "multiple players retrieved correctly" ) {
        location loc{0, "test", 0, 0};
        locations_repo.insert(loc, transaction);

        user usr{0, "user", "pass", "email", 0, "code", 0, 0};
        users_repo.insert_if_not_exists(usr, transaction);

        player plyr{0, usr.id, loc.id, "john doe"s, {}, {}, {}};
        player plyr2{0, usr.id, loc.id, "john doe2"s, {}, {}, {}};
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