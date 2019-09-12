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

#include "load_map.h"
#include <filesystem>
#include <rapidjson/document.h>
#include <yaml-cpp/yaml.h>
#include <working_directory_manipulation.h>
#include "spdlog/spdlog.h"
#include <range/v3/view/remove_if.hpp>
#include <entt/entity/registry.hpp>

using namespace std;
using namespace rapidjson;
using namespace lotr;

#define SPAWN_STRING_FIELD(x) if(tree[ #x ]) { script. x = tree[ #x ].as<string>(); }
#define SPAWN_UINT_FIELD(x) if(tree[ #x ]) { script. x = tree[ #x ].as<uint32_t>(); }
#define SPAWN_BOOL_FIELD(x) if(tree[ #x ]) { script. x = tree[ #x ].as<bool>(); }

uint32_t spawner_id_counter = 0;

optional<spawner_script> create_spawner_script_for_npc(rapidjson::Value &current_object, uint32_t x, uint32_t y, uint32_t first_gid) {
    spawner_script script;

    script.id = spawner_id_counter++;
    script.leash_radius = -1;
    script.initial_spawn = 1;
    script.max_creatures = 1;
    script.spawn_radius = 0;
    script.x = x;
    script.y = y;

    script.npc_ids.emplace_back(1, current_object["name"].GetString());
    script.npc_ids[0].sprite.push_back(current_object["gid"].GetUint() - first_gid);
    for(auto &stat : stats) {
        script.npc_ids[0].stats.emplace_back(stat, 10);
    }

    return script;
}

optional<spawner_script> get_spawner_script(string const &script_file, uint32_t x, uint32_t y, entt::registry &registry) {
    string actual_script_file = fmt::format("assets/scripts/spawners/{}.yml", script_file);
    spawner_script script;

    spdlog::debug("[{}] loading spawner script {}", __FUNCTION__, actual_script_file);

    auto env_contents = read_whole_file(actual_script_file);

    if(!env_contents) {
        return {};
    }

    YAML::Node tree = YAML::Load(env_contents.value());

    script.id = spawner_id_counter++;
    script.respawn_rate = tree["respawnRate"].as<uint32_t>();
    script.initial_spawn = tree["initialSpawn"].as<uint32_t>();
    script.max_creatures = tree["maxCreatures"].as<uint32_t>();
    script.spawn_radius = tree["spawnRadius"].as<uint32_t>();
    script.random_walk_radius = tree["randomWalkRadius"].as<uint32_t>();
    script.leash_radius = tree["leashRadius"].as<uint32_t>();
    script.x = x;
    script.y = y;

    SPAWN_UINT_FIELD(elite_tick_cap)
    SPAWN_UINT_FIELD(max_spawn)

    SPAWN_BOOL_FIELD(should_be_active)
    SPAWN_BOOL_FIELD(can_slow_down)
    SPAWN_BOOL_FIELD(should_serialize)
    SPAWN_BOOL_FIELD(always_spawn)
    SPAWN_BOOL_FIELD(require_dead_to_respawn)
    SPAWN_BOOL_FIELD(do_initial_spawn_immediately)

    for(auto const &kv : tree["npcIds"]) {
        auto npc_id = kv["name"].as<string>();
        auto chance = kv["chance"].as<uint32_t>();
        spdlog::trace("[{}] npc id {} {}", __FUNCTION__, npc_id, chance);

        auto gnpc = lotr::get_global_npc_by_npc_id(registry, npc_id);

        if(gnpc == nullptr) {
            spdlog::error("[{}] npc id {} not found in global npcs", __FUNCTION__, npc_id);
            continue;
        }

        script.npc_ids.emplace_back(chance, *gnpc);
    }

    for(auto const &kv : tree["paths"]) {
        spdlog::trace("[{}] npc path {}", __FUNCTION__, kv.as<string>());
        script.paths.push_back(kv.as<string>());
    }

    if(tree["npcAISettings"]) {
        for (auto const &kv : tree["npcAISettings"]) {
            spdlog::trace("[{}] npc ai setting {}", __FUNCTION__, kv.as<string>());
            script.npc_ai_settings.push_back(kv.as<string>());
        }
    }

    return script;
}

vector<map_property> get_properties(Value const &properties) {
    vector<map_property> object_properties;

    for (auto it = properties.MemberBegin(); it != properties.MemberEnd(); it++) {
        string name = it->name.GetString();
        if (it->value.GetType() == Type::kStringType) {
            object_properties.emplace_back(name, string(it->value.GetString()));
        }

        if (it->value.GetType() == Type::kNumberType) {
            object_properties.emplace_back(name, it->value.GetDouble());
        }
    }

    return object_properties;
}

optional<map_component> lotr::load_map_from_file(const string &file, entt::registry &registry) {
    spdlog::debug("[{}] Loading load_map {}", __FUNCTION__, file);
    auto env_contents = read_whole_file(file);

    if(!env_contents) {
        return {};
    }

    Document d;
    d.Parse(env_contents->c_str(), env_contents->size());

    if (d.HasParseError() || !d.IsObject()) {
        spdlog::error("[{}] deserialize config.json failed", __FUNCTION__);
        return {};
    }

    uint32_t width = d["width"].GetUint();
    uint32_t height = d["height"].GetUint();
    uint32_t tilewidth = d["tilewidth"].GetUint();
    uint32_t tileheight = d["tileheight"].GetUint();
    string map_name = filesystem::path(file).filename();
    map_name = map_name.substr(0, map_name.size() - 5);

    auto& json_tilesets = d["tilesets"];
    vector<map_tileset> map_tilesets;

    for(SizeType i = 0; i < json_tilesets.Size(); i++) {
        auto& current_tileset = json_tilesets[i];
        spdlog::trace("[{}] tileset {}: {}", __FUNCTION__, i, current_tileset["name"].GetString());

        map_tilesets.emplace_back(current_tileset["firstgid"].GetUint(), current_tileset["name"].GetString());
    }

    vector<map_layer> map_layers;
    auto& json_layers = d["layers"];

    for(SizeType i = 0; i < json_layers.Size(); i++) {
        auto& current_layer = json_layers[i];
        spdlog::trace("[{}] layer {}: {}", __FUNCTION__, i, current_layer["name"].GetString());


        if (!current_layer.IsObject()) {
            spdlog::warn("[{}] deserialize failed", __FUNCTION__);
            return nullopt;
        }

        if(current_layer["type"].GetString() == tile_layer_name) {
            vector<uint32_t> data;
            for (SizeType j = 0; j < current_layer["data"].Size(); j++) {
                data.push_back(current_layer["data"][j].GetUint());
            }
            map_layers.emplace_back(current_layer["x"].GetInt(), current_layer["y"].GetInt(), current_layer["width"].GetInt(),
                                    current_layer["height"].GetInt(), current_layer["name"].GetString(), current_layer["type"].GetString(), vector<map_object>{}, data);
        }

        string current_layer_name = current_layer["name"].GetString();
        if(current_layer["type"].GetString() == object_layer_name) {
            vector<map_object> objects;
            objects.reserve(width*height);

            for(uint32_t i2 = 0; i2 < width*height; i2++) {
                objects.emplace_back();
            }

            for (SizeType j = 0; j < current_layer["objects"].Size(); j++) {
                auto& current_object = current_layer["objects"][j];
                //spdlog::trace("[{}] object {}", __FUNCTION__, current_object["id"].GetUint());

                vector<map_property> object_properties;
                if(current_object.HasMember("properties")) {
                    object_properties = get_properties(current_object["properties"]);
                }

                uint32_t gid = 0;
                if(current_object.HasMember("gid")) {
                    gid = current_object["gid"].GetUint();
                }

                optional<spawner_script> spawn_script;
                uint32_t x = current_object["x"].GetUint() / tilewidth;
                uint32_t y = (current_object["y"].GetUint() - tileheight) / tileheight; // something weird about tiled putting an object at offset Y coords
                auto object_script_property = find_if(begin(object_properties), end(object_properties), [](map_property const &prop) noexcept {return prop.name == "script";});
                if((current_layer_name == spawners_layer_name) && object_script_property != end(object_properties)) {
                    spawn_script = get_spawner_script(get<string>(object_script_property->value), x, y, registry);

                    auto object_resource_name_property = find_if(begin(object_properties), end(object_properties), [](map_property const &prop) noexcept {return prop.name == "resourceName";});
                    if(object_resource_name_property != end(object_properties)) {
                        auto gnpc = lotr::get_global_npc_by_npc_id(registry, get<string>(object_resource_name_property->value));

                        if(gnpc != nullptr) {
                            spawn_script->npc_ids.emplace_back(1, *gnpc);
                        } else {
                            spdlog::error("[{}] npc id {} not found in global npcs", __FUNCTION__, get<string>(object_resource_name_property->value));
                        }
                    }
                }

                if(current_layer_name == npcs_layer_name) {
                    auto test = map_tilesets | ranges::views::remove_if([&](map_tileset const &t){ return t.firstgid > current_object["gid"].GetUint(); });

                    spawn_script = create_spawner_script_for_npc(current_object, x, y, test.back().firstgid);
                    spdlog::trace("[{}] npc {} assigned spawner", __FUNCTION__, current_object["name"].GetString());
                }

                objects[x + y * width] = map_object(gid, current_object["id"].GetUint(),
                        x, y, current_object["width"].GetUint(),
                        current_object["height"].GetUint(), current_object["name"].GetString(), current_object["type"].GetString(), object_properties, spawn_script);
            }
            map_layers.emplace_back(current_layer["x"].GetInt(), current_layer["y"].GetInt(), 0, 0, current_layer["name"].GetString(), current_layer["type"].GetString(),
                                    move(objects), vector<uint32_t>{});
        }
    }

    vector<map_property> map_properties = get_properties(d["properties"]);

    spdlog::trace("[{}] map {} {} {}", __FUNCTION__, width, height, map_name);
    return make_optional<map_component>(width, height, move(map_name), move(map_properties), move(map_layers), move(map_tilesets));
}

