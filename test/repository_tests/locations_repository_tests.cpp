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
#include "repositories/locations_repository.h"

using namespace std;
using namespace lotr;

TEST_CASE("locations repository tests") {
    locations_repository<database_pool, database_transaction> loc_repo(db_pool);

    SECTION( "location inserted correctly" ) {
        auto transaction = loc_repo.create_transaction();
        db_location loc{0, "load_map name", 10, 10};
        loc_repo.insert(loc, transaction);
        REQUIRE(loc.id != 0);

        auto loc2 = loc_repo.get(loc.id, transaction);
        REQUIRE(loc2->id == loc.id);
        REQUIRE(loc2->map_name == loc.map_name);
        REQUIRE(loc2->x == loc.x);
        REQUIRE(loc2->y == loc.y);
    }

    SECTION( "update location" ) {
        auto transaction = loc_repo.create_transaction();
        db_location loc{0, "load_map name", 10, 10};
        loc_repo.insert(loc, transaction);
        REQUIRE(loc.id != 0);

        loc.map_name = "map2";
        loc.x = 11;
        loc.y = 11;

        loc_repo.update(loc, transaction);

        auto loc2 = loc_repo.get(loc.id, transaction);
        REQUIRE(loc2->id == loc.id);
        REQUIRE(loc2->map_name == loc.map_name);
        REQUIRE(loc2->x == loc.x);
        REQUIRE(loc2->y == loc.y);
    }
}

#endif
