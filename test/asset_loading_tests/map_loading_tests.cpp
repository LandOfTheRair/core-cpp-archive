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

#include "asset_loading/load_map.h"

using namespace std;
using namespace lotr;

TEST_CASE("map_loading tests") {
    ofstream map_file("test_map.json", ios_base::trunc);
    map_file << R"({
                    "height":48,
                    "width":48,
                    "tileheight":64,
                    "tilewidth":64,
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
                                "script":"tutorial\/townee"
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
                    },{
                        "draworder":"topdown",
                        "name":"NPCs",
                        "objects":[{
                            "gid":3183,
                            "height":64,
                            "id":175,
                            "name":"Weapon Seller",
                            "properties": {
                                "script":"tutorial\/crier"
                            },
                            "propertytypes": {
                                "script":"string"
                            },
                            "rotation":0,
                            "type":"test",
                            "visible":true,
                            "width":64,
                            "x":1536,
                            "y":2240
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
                    },
                    "tilesets":[{
                        "columns":24,
                        "firstgid":1,
                        "image":"..\/..\/client\/assets\/spritesheets\/terrain.png",
                        "imageheight":2560,
                        "imagewidth":1536,
                        "margin":0,
                        "name":"Terrain",
                        "spacing":0,
                        "terrains": [],
                        "tilecount":960,
                        "tileheight":64,
                        "tiles": [],
                        "tilewidth":64
                    }]})";

    map_file.close();

    entt::registry registry;
    auto gnpc_entity = registry.create();
    global_npc_component gnpc;
    gnpc.npc_id = "Tutorial Townee";
    registry.assign<global_npc_component>(gnpc_entity, gnpc);

    auto map = load_map_from_file("test_map.json", registry);
    REQUIRE(map);
    REQUIRE(map->name == "test_map"s);
    REQUIRE(map->width == 48);
    REQUIRE(map->height == 48);
    REQUIRE(map->layers.size() == 3);
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
    REQUIRE(map->layers[1].objects.size() == 48*48);
    REQUIRE(map->layers[1].data.size() == 0);

    REQUIRE(map->layers[2].width == 0);
    REQUIRE(map->layers[2].height == 0);
    REQUIRE(map->layers[2].x == 0);
    REQUIRE(map->layers[2].y == 0);
    REQUIRE(map->layers[2].name == "NPCs"s);
    REQUIRE(map->layers[2].type == "objectgroup"s);
    REQUIRE(map->layers[2].objects.size() == 48*48);
    REQUIRE(map->layers[2].data.size() == 0);

    auto const &object = map->layers[1].objects[1472/64 + (2176 - 64)/64 * 48];
    REQUIRE(object.gid == 3182);
    REQUIRE(object.id == 174);
    REQUIRE(object.x == 23);
    REQUIRE(object.y == 33);
    REQUIRE(object.width == 64);
    REQUIRE(object.height == 64);
    REQUIRE(object.name == "Armor Seller"s);
    REQUIRE(object.type == "test"s);
    REQUIRE(object.properties.size() == 1);
    REQUIRE(object.properties[0].name == "script");
    REQUIRE(get<string>(object.properties[0].value) == "tutorial/townee");

    auto const &object2 = map->layers[2].objects[1536/64 + (2240 - 64)/64 * 48];
    REQUIRE(object2.gid == 3183);
    REQUIRE(object2.id == 175);
    REQUIRE(object2.x == 24);
    REQUIRE(object2.y == 34);
    REQUIRE(object2.width == 64);
    REQUIRE(object2.height == 64);
    REQUIRE(object2.name == "Weapon Seller"s);
    REQUIRE(object2.type == "test"s);
    REQUIRE(object2.properties.size() == 1);
    REQUIRE(object2.properties[0].name == "script");
    REQUIRE(get<string>(object2.properties[0].value) == "tutorial/crier");

    REQUIRE(object.script);
    REQUIRE(object.script->npc_ids.size() == 1);
    REQUIRE(object.script->npc_ids[0].npc_id == "Tutorial Townee");
    REQUIRE(object.script->npc_ids[0].chance == 1);
    REQUIRE(object.script->respawn_rate == 15);
    REQUIRE(object.script->initial_spawn == 2);
    REQUIRE(object.script->max_creatures == 20);
    REQUIRE(object.script->spawn_radius == 0);
    REQUIRE(object.script->random_walk_radius == 0);
    REQUIRE(object.script->leash_radius == 30);

    REQUIRE(object.script->paths.size() == 4);
    REQUIRE(object.script->paths[0] == "23-E 16-S 23-W 16-N"s);
    REQUIRE(object.script->paths[1] == "16-S 23-E 16-N 23-W"s);
    REQUIRE(object.script->paths[2] == "8-S 23-E 8-S 23-W 8-N 23-E 8-N 23-W"s);
    REQUIRE(object.script->paths[3] == "23-E 8-S 23-W 8-S 23-E 8-N 23-W 8-N"s);

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
