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
#include <spdlog/spdlog.h>
#include "test_helpers/startup_helper.h"
#include "game_logic/fov.h"
#include <ecs/components.h>

using namespace std;
using namespace lotr;

TEST_CASE("fov tests") {
    SECTION("no obstacles") {
        const uint32_t map_size = 9;
        array<map_layer, 15> map_layers;
        vector<uint32_t> wall_data(map_size*map_size);
        vector<map_object> object_data(map_size*map_size);

        for(uint32_t i = 0; i < map_size*map_size; i++) {
            wall_data.push_back(0);
            object_data.emplace_back(0, 0, 0, 0, "", "", vector<map_property>{}, nullopt);
        }

        map_layers[map_layer_name::Walls] = map_layer(0, 0, map_size, map_size, "wall_layer_name", "tilelayer"s, vector<map_object>{}, wall_data);
        map_layers[map_layer_name::OpaqueDecor] = map_layer(0, 0, map_size, map_size, "opaque_layer_name", "objectgroup"s, object_data, vector<uint32_t>{});

        map_component m(map_size, map_size, "test"s, {}, map_layers, {});
        auto loc = make_tuple(4, 4);

        auto fov = compute_fov_restrictive_shadowcasting(m, loc, false);

        REQUIRE(fov.all());
    }

    SECTION("walls encage db_character") {
        const uint32_t map_size = 9;
        array<map_layer, 15> map_layers;
        vector<uint32_t> wall_data(map_size*map_size);
        vector<map_object> object_data(map_size*map_size);

        for(uint32_t i = 0; i < map_size*map_size; i++) {
            wall_data.push_back(0);
            object_data.emplace_back(0, 0, 0, 0, "", "", vector<map_property>{}, nullopt);
        }

        wall_data[3+3*map_size] = 1;
        wall_data[4+3*map_size] = 1;
        wall_data[5+3*map_size] = 1;
        wall_data[3+4*map_size] = 1;
        wall_data[5+4*map_size] = 1;
        wall_data[3+5*map_size] = 1;
        wall_data[4+5*map_size] = 1;
        wall_data[5+5*map_size] = 1;

        map_layers[map_layer_name::Walls] = map_layer(0, 0, map_size, map_size, "wall_layer_name", "tilelayer"s, vector<map_object>{}, wall_data);
        map_layers[map_layer_name::OpaqueDecor] = map_layer(0, 0, map_size, map_size, "opaque_layer_name", "objectgroup"s, object_data, vector<uint32_t>{});

        map_component m(map_size, map_size, "test"s, {}, map_layers, {});
        auto loc = make_tuple(4, 4);

        auto fov = compute_fov_restrictive_shadowcasting(m, loc, false);

        for(uint32_t y = 0; y < fov_diameter; y++) {
            for(uint32_t x = 0; x < fov_diameter; x++) {
                if(x == 4 && y == 4) {
                    REQUIRE(fov[x + y * fov_diameter] == true);
                } else {
                    REQUIRE(fov[x + y * fov_diameter] == false);
                }
            }
        }
    }

    SECTION("opaque encage db_character") {
        const uint32_t map_size = 9;
        array<map_layer, 15> map_layers;
        vector<uint32_t> wall_data(map_size*map_size);
        vector<map_object> object_data(map_size*map_size);

        for(uint32_t i = 0; i < map_size*map_size; i++) {
            wall_data.push_back(0);
            object_data.emplace_back(0, 0, 0, 0, "", "", vector<map_property>{}, nullopt);
        }

        object_data[3+3*map_size] = map_object(1, 0, 64, 64, "test"s, "test"s, vector<map_property>{}, nullopt);
        object_data[4+3*map_size] = map_object(1, 0, 64, 64, "test"s, "test"s, vector<map_property>{}, nullopt);
        object_data[5+3*map_size] = map_object(1, 0, 64, 64, "test"s, "test"s, vector<map_property>{}, nullopt);
        object_data[3+4*map_size] = map_object(1, 0, 64, 64, "test"s, "test"s, vector<map_property>{}, nullopt);
        object_data[5+4*map_size] = map_object(1, 0, 64, 64, "test"s, "test"s, vector<map_property>{}, nullopt);
        object_data[3+5*map_size] = map_object(1, 0, 64, 64, "test"s, "test"s, vector<map_property>{}, nullopt);
        object_data[4+5*map_size] = map_object(1, 0, 64, 64, "test"s, "test"s, vector<map_property>{}, nullopt);
        object_data[5+5*map_size] = map_object(1, 0, 64, 64, "test"s, "test"s, vector<map_property>{}, nullopt);

        map_layers[map_layer_name::Walls] = map_layer(0, 0, map_size, map_size, "wall_layer_name", "tilelayer"s, vector<map_object>{}, wall_data);
        map_layers[map_layer_name::OpaqueDecor] = map_layer(0, 0, map_size, map_size, "opaque_layer_name", "objectgroup"s, object_data, vector<uint32_t>{});

        map_component m(map_size, map_size, "test"s, {}, map_layers, {});
        auto loc = make_tuple(4, 4);

        auto fov = compute_fov_restrictive_shadowcasting(m, loc, false);

        for(uint32_t y = 0; y < fov_diameter; y++) {
            for(uint32_t x = 0; x < fov_diameter; x++) {
                if(x == 4 && y == 4) {
                    REQUIRE(fov[x + y * fov_diameter] == true);
                } else {
                    REQUIRE(fov[x + y * fov_diameter] == false);
                }
            }
        }
    }

    SECTION("single object") {
        const uint32_t map_size = 9;
        array<map_layer, 15> map_layers;
        vector<uint32_t> wall_data(map_size*map_size);
        vector<map_object> object_data(map_size*map_size);

        for(uint32_t i = 0; i < map_size*map_size; i++) {
            wall_data.push_back(0);
            object_data.emplace_back(0, 0, 0, 0, "", "", vector<map_property>{}, nullopt);
        }

        wall_data[4+3*map_size] = 1;

        map_layers[map_layer_name::Walls] = map_layer(0, 0, map_size, map_size, "wall_layer_name", "tilelayer"s, vector<map_object>{}, wall_data);
        map_layers[map_layer_name::OpaqueDecor] = map_layer(0, 0, map_size, map_size, "opaque_layer_name", "objectgroup"s, object_data, vector<uint32_t>{});

        map_component m(map_size, map_size, "test"s, {}, map_layers, {});
        auto loc = make_tuple(4, 4);

        auto fov = compute_fov_restrictive_shadowcasting(m, loc, false);

        bitset<power(fov_diameter)> check_fov("111111111111111111111111111111111111111111111111101111111101111111000111111000111"s);

        REQUIRE(fov == check_fov);
    }

    SECTION("corners") {
        const uint32_t map_size = 9;
        array<map_layer, 15> map_layers;
        vector<uint32_t> wall_data(map_size*map_size);
        vector<map_object> object_data(map_size*map_size);

        for(uint32_t i = 0; i < map_size*map_size; i++) {
            wall_data.push_back(0);
            object_data.emplace_back(0, 0, 0, 0, "", "", vector<map_property>{}, nullopt);
        }

        map_layers[map_layer_name::Walls] = map_layer(0, 0, map_size, map_size, "wall_layer_name", "tilelayer"s, vector<map_object>{}, wall_data);
        map_layers[map_layer_name::OpaqueDecor] = map_layer(0, 0, map_size, map_size, "opaque_layer_name", "objectgroup"s, object_data, vector<uint32_t>{});

        map_component m(map_size, map_size, "test"s, {}, map_layers, {});
        auto loc1 = make_tuple(0, 0);
        auto loc2 = make_tuple(8, 0);
        auto loc3 = make_tuple(0, 8);
        auto loc4 = make_tuple(8, 8);

        auto fov = compute_fov_restrictive_shadowcasting(m, loc1, false);
        bitset<power(fov_diameter)> check_fov("111110000111110000111110000111110000111110000000000000000000000000000000000000000"s);
        REQUIRE(fov == check_fov);

        fov = compute_fov_restrictive_shadowcasting(m, loc2, false);
        check_fov = bitset<power(fov_diameter)>("000011111000011111000011111000011111000011111000000000000000000000000000000000000"s);
        REQUIRE(fov == check_fov);

        fov = compute_fov_restrictive_shadowcasting(m, loc3, false);
        check_fov = bitset<power(fov_diameter)>("000000000000000000000000000000000000111110000111110000111110000111110000111110000"s);
        REQUIRE(fov == check_fov);

        fov = compute_fov_restrictive_shadowcasting(m, loc4, false);
        check_fov = bitset<power(fov_diameter)>("000000000000000000000000000000000000000011111000011111000011111000011111000011111"s);
        REQUIRE(fov == check_fov);
    }
}
