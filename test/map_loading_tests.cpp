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
#include <iostream>
#include <fstream>

#include "test_helpers/startup_helper.h"
#include "../src/map_loading/load_map.h"

using namespace std;
using namespace lotr;

TEST_CASE("map_loading tests") {
    ofstream map_file("test_map.json", ios_base::trunc);
    map_file << R"({
                    "height":48,
                    "width":48,
                    "layers":[{
                        "data": [1, 2, 3, 4, 5, 6],
                        "height":48,
                        "name":"Terrain",
                        "opacity":1,
                        "type":"tilelayer",
                        "visible":true,
                        "width":48,
                        "x":0,
                        "y":0
                    },{
                        "draworder":"topdown",
                        "name":"Spawners",
                        "objects":[{
                            "gid":3182,
                            "height":64,
                            "id":174,
                            "name":"Armor Seller",
                            "properties": {
                                "script":"tutorial\/monster"
                            },
                            "propertytypes": {
                                "script":"string"
                            },
                            "rotation":0,
                            "type":"test",
                            "visible":true,
                            "width":64,
                            "x":1472,
                            "y":2176
                        }],
                        "opacity":1,
                        "type":"objectgroup",
                        "visible":true,
                        "x":0,
                        "y":0
                    }],
                    "properties": {
                        "itemExpirationHours":1,
                        "maxSkill":3,
                        "region":"Tutorial",
                        "respawnX":14,
                        "respawnY":14
                    }})";

    map_file.close();

    auto map = load_map_from_file("test_map.json");
    REQUIRE(map);
    REQUIRE(map->name == "test_map.json"s);
    REQUIRE(map->width == 48);
    REQUIRE(map->height == 48);
    REQUIRE(map->layers.size() == 2);
    REQUIRE(map->layers[0].width == 48);
    REQUIRE(map->layers[0].height == 48);
    REQUIRE(map->layers[0].x == 0);
    REQUIRE(map->layers[0].y == 0);
    REQUIRE(map->layers[0].name == "Terrain"s);
    REQUIRE(map->layers[0].type == "tilelayer"s);
    REQUIRE(map->layers[0].objects.size() == 0);

    REQUIRE(map->layers[0].data.size() == 6);
    REQUIRE(map->layers[0].data[0] == 1);
    REQUIRE(map->layers[0].data[1] == 2);
    REQUIRE(map->layers[0].data[2] == 3);
    REQUIRE(map->layers[0].data[3] == 4);
    REQUIRE(map->layers[0].data[4] == 5);
    REQUIRE(map->layers[0].data[5] == 6);

    REQUIRE(map->layers[1].width == 0);
    REQUIRE(map->layers[1].height == 0);
    REQUIRE(map->layers[1].x == 0);
    REQUIRE(map->layers[1].y == 0);
    REQUIRE(map->layers[1].name == "Spawners"s);
    REQUIRE(map->layers[1].type == "objectgroup"s);
    REQUIRE(map->layers[1].objects.size() == 1);
    REQUIRE(map->layers[1].data.size() == 0);

    REQUIRE(map->layers[1].objects[0].gid == 3182);
    REQUIRE(map->layers[1].objects[0].id == 174);
    REQUIRE(map->layers[1].objects[0].x == 1472);
    REQUIRE(map->layers[1].objects[0].y == 2176);
    REQUIRE(map->layers[1].objects[0].width == 64);
    REQUIRE(map->layers[1].objects[0].height == 64);
    REQUIRE(map->layers[1].objects[0].name == "Armor Seller"s);
    REQUIRE(map->layers[1].objects[0].type == "test"s);
    REQUIRE(map->layers[1].objects[0].properties.size() == 1);
    REQUIRE(map->layers[1].objects[0].properties[0].name == "script");
    REQUIRE(get<string>(map->layers[1].objects[0].properties[0].value) == "tutorial/monster");
    REQUIRE(map->layers[1].objects[0].script);
    REQUIRE(map->layers[1].objects[0].script->npc_ids.size() == 2);
    REQUIRE(map->layers[1].objects[0].script->npc_ids[0].name == "Tutorial Deer");
    REQUIRE(map->layers[1].objects[0].script->npc_ids[0].chance == 0);
    REQUIRE(map->layers[1].objects[0].script->npc_ids[1].name == "Tutorial Wolf");
    REQUIRE(map->layers[1].objects[0].script->npc_ids[1].chance == 0);
    REQUIRE(map->layers[1].objects[0].script->respawn_rate == 15);
    REQUIRE(map->layers[1].objects[0].script->initial_spawn == 2);
    REQUIRE(map->layers[1].objects[0].script->max_creatures == 6);
    REQUIRE(map->layers[1].objects[0].script->spawn_radius == 1);
    REQUIRE(map->layers[1].objects[0].script->random_walk_radius == 5);
    REQUIRE(map->layers[1].objects[0].script->leash_radius == 10);


    REQUIRE(map->properties.size() == 5);
    REQUIRE(map->properties[0].name == "itemExpirationHours"s);
    REQUIRE(get<double>(map->properties[0].value) == 1);
    REQUIRE(map->properties[1].name == "maxSkill"s);
    REQUIRE(get<double>(map->properties[1].value) == 3);
    REQUIRE(map->properties[2].name == "region"s);
    REQUIRE(get<string>(map->properties[2].value) == "Tutorial"s);
    REQUIRE(map->properties[3].name == "respawnX"s);
    REQUIRE(get<double>(map->properties[3].value) == 14);
    REQUIRE(map->properties[4].name == "respawnY"s);
    REQUIRE(get<double>(map->properties[4].value) == 14);
}