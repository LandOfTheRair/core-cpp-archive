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
#include "../test_helpers/startup_helper.h"
#include <game_logic/a_star.h>
#include <spdlog/spdlog.h>

using namespace std;
using namespace lotr;

TEST_CASE("a star tests") {
    uint32_t const map_size = 10;

    SECTION( "no objects" ) {
        lotr_flat_map<map_layer_name, map_layer> layers;
        {
            vector<map_object> objects;
            vector<uint32_t> data;

            for (uint32_t x = 0; x < 10; x++) {
                for (uint32_t y = 0; y < 10; y++) {
                    data.push_back(0);
                    objects.emplace_back();
                }
            }

            layers[map_layer_name::Walls] = map_layer(0, 0, map_size, map_size, wall_layer_name, ""s, vector<map_object>{}, move(data));
            layers[map_layer_name::OpaqueDecor] = map_layer(0, 0, map_size, map_size, opaque_layer_name, ""s, move(objects), vector<uint32_t>{});
        }

        map_component test_map(map_size, map_size, "test", {}, move(layers), {});

        auto start = make_tuple(1, 1);
        auto goal = make_tuple(9, 9);

        auto path = a_star_path(test_map, start, goal);

        auto &next = path[goal];
        int steps = 0;
        while(next != start) {
            REQUIRE(path.find(next) != end(path));
            next = path[next];
            steps++;
        }
        steps++;

        REQUIRE(steps == 8);
    }

    SECTION( "some objects" ) {
        lotr_flat_map<map_layer_name, map_layer> layers;
        {
            vector<map_object> objects;
            vector<uint32_t> data;

            for (uint32_t x = 0; x < 10; x++) {
                for (uint32_t y = 0; y < 10; y++) {
                    if((x == 4 || x == 5 || x == 6) && y == 5) {
                        data.push_back(1);
                    } else {
                        data.push_back(0);
                    }
                    objects.emplace_back();
                }
            }

            layers[map_layer_name::Walls] = map_layer(0, 0, map_size, map_size, wall_layer_name, ""s, vector<map_object>{}, move(data));
            layers[map_layer_name::OpaqueDecor] = map_layer(0, 0, map_size, map_size, opaque_layer_name, ""s, move(objects), vector<uint32_t>{});
        }

        map_component test_map(map_size, map_size, "test", {}, move(layers), {});

        auto start = make_tuple(1, 1);
        auto goal = make_tuple(9, 9);

        auto path = a_star_path(test_map, start, goal);

        auto &next = path[goal];
        int steps = 0;
        while(next != start) {
            REQUIRE(path.find(next) != end(path));
            next = path[next];
            steps++;
        }
        steps++;

        REQUIRE(steps == 10);
    }
}
