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
#include <range/v3/all.hpp>
#include <entt/entity/registry.hpp>
#include <charconv>

using namespace std;
using namespace rapidjson;
using namespace lotr;

#define SPAWN_STRING_FIELD(x) if(tree[ #x ]) { script. x = tree[ #x ].as<string>(); }
#define SPAWN_UINT_FIELD(x) if(tree[ #x ]) { script. x = tree[ #x ].as<uint32_t>(); }
#define SPAWN_BOOL_FIELD(x) if(tree[ #x ]) { script. x = tree[ #x ].as<bool>(); }

uint32_t spawner_id_counter = 0;

[[nodiscard]]
optional<spawner_script> create_spawner_script_for_npc(rapidjson::Value &current_object, uint32_t x, uint32_t y, uint32_t first_gid) {
    spawner_script script;

    script.id = spawner_id_counter++;
    script.leash_radius = -1;
    script.initial_spawn = 1;
    script.max_creatures = 1;
    script.spawn_radius = 0;
    script.loc = make_tuple(x, y);

    script.npc_ids.emplace_back(1, current_object["name"].GetString());
    script.npc_ids[0].sprite.push_back(current_object["gid"].GetUint() - first_gid);
    script.npc_ids[0].stats.reserve(stat_names.size());
    for(auto &stat : stat_names) {
        script.npc_ids[0].stats.emplace_back(stat, 10);
    }

    return script;
}

[[nodiscard]]
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
    script.loc = make_tuple(x, y);

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
        string path = kv.as<string>();
        spdlog::trace("[{}] npc path {}", __FUNCTION__, path);
        vector<string_view> split_path = path | ranges::views::split(' ') | ranges::views::transform([](auto &&rng){return string_view(&*rng.begin(), ranges::distance(rng));}) | ranges::to_vector;
        vector<npc_path> spawn_paths;
        for(auto split : split_path) {
            vector<string_view> temp = split | ranges::views::split('-') | ranges::views::transform([](auto &&rng){return string_view(&*rng.begin(), ranges::distance(rng));}) | ranges::to_vector;
            if(temp.size() != 2) {
                spdlog::error("[{}] npc file {} path {} split {} temp size not equal to 2 but is {}", __FUNCTION__, actual_script_file, path, split, temp.size());
                continue;
            }
            uint32_t value;
            if(auto [p, ec] = from_chars(temp[0].data(), temp[0].data() + temp[0].size(), value); ec != errc()) {
                spdlog::error("[{}] npc file {} path {} split {} couldn't convert to integer", __FUNCTION__, actual_script_file, path, split);
                continue;
            }
            movement_direction dir;
            if(temp[1] == north_direction) {
                dir = movement_direction::North;
            } else if(temp[1] == east_direction) {
                dir = movement_direction::East;
            } else if(temp[1] == south_direction) {
                dir = movement_direction::South;
            } else if(temp[1] == west_direction)  {
                dir = movement_direction::West;
            } else if(temp[1] == north_east_direction)  {
                dir = movement_direction::NorthEast;
            } else if(temp[1] == north_west_direction)  {
                dir = movement_direction::NorthWest;
            } else if(temp[1] == south_east_direction)  {
                dir = movement_direction::SouthEast;
            } else if(temp[1] == south_west_direction)  {
                dir = movement_direction::SouthWest;
            } else {
                spdlog::error("[{}] npc file {} could not decode path {} split {} missing direction {}", __FUNCTION__, actual_script_file, path, split, temp[1]);
                continue;
            }
            spawn_paths.emplace_back(dir, value);
        }
        script.paths.emplace_back(move(spawn_paths));
    }

    if(tree["npcAISettings"]) {
        for (auto const &kv : tree["npcAISettings"]) {
            spdlog::trace("[{}] npc ai setting {}", __FUNCTION__, kv.as<string>());
            script.npc_ai_settings.push_back(kv.as<string>());
        }
    }

    return script;
}

[[nodiscard]]
vector<map_property> get_properties(Value const &properties) {
    vector<map_property> object_properties;
    object_properties.reserve(properties.MemberCount());

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

[[nodiscard]]
map_layer_name layer_name_to_enum(string const &name) {
    if(name == "Terrain") {
        return map_layer_name::Terrain;
    } else if(name == "Floors") {
        return map_layer_name::Floors;
    } else if(name == "Fluids") {
        return map_layer_name::Fluids;
    } else if(name == "Foliage") {
        return map_layer_name::Foliage;
    } else if(name == "Walls") {
        return map_layer_name::Walls;
    } else if(name == "Decor") {
        return map_layer_name::Decor;
    } else if(name == "DenseDecor") {
        return map_layer_name::DenseDecor;
    } else if(name == "OpaqueDecor") {
        return map_layer_name::OpaqueDecor;
    } else if(name == "Interactables") {
        return map_layer_name::Interactables;
    } else if(name == "NPCs") {
        return map_layer_name::NPCs;
    } else if(name == "Spawners") {
        return map_layer_name::Spawners;
    } else if(name == "RegionDescriptions") {
        return map_layer_name::RegionDescriptions;
    } else if(name == "BackgroundMusic") {
        return map_layer_name::BackgroundMusic;
    } else if(name == "Succorport") {
        return map_layer_name::Succorport;
    } else if(name == "SpawnerZones") {
        return map_layer_name::SpawnerZones;
    }

    spdlog::error("[{}] code path should never be hit", __FUNCTION__);
    return map_layer_name::SpawnerZones;
}

optional<map_component> lotr::load_map_from_file(const string &file, entt::registry &registry, lotr_flat_map <string, optional<spawner_script>> &spawner_script_cache) {
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
    map_tilesets.reserve(json_tilesets.Size());

    for(SizeType i = 0; i < json_tilesets.Size(); i++) {
        auto& current_tileset = json_tilesets[i];
        spdlog::trace("[{}] tileset {}: {}", __FUNCTION__, i, current_tileset["name"].GetString());

        map_tilesets.emplace_back(current_tileset["firstgid"].GetUint(), current_tileset["name"].GetString());
    }

    array<map_layer, 15> map_layers;
    auto& json_layers = d["layers"];

    for(SizeType i = 0; i < json_layers.Size(); i++) {
        auto& current_layer = json_layers[i];
        string current_layer_name = current_layer["name"].GetString();
        spdlog::trace("[{}] layer {}: {}", __FUNCTION__, i, current_layer_name);
        auto current_layer_enum = layer_name_to_enum(current_layer_name);

        if (!current_layer.IsObject()) {
            spdlog::warn("[{}] deserialize failed", __FUNCTION__);
            return nullopt;
        }

        if(current_layer["type"].GetString() == "tilelayer"s) {
            vector<uint32_t> data;
            for (SizeType j = 0; j < current_layer["data"].Size(); j++) {
                data.push_back(current_layer["data"][j].GetUint());
            }
            map_layers[current_layer_enum] = map_layer(current_layer["x"].GetInt(), current_layer["y"].GetInt(), current_layer["width"].GetInt(),
                                                   current_layer["height"].GetInt(), current_layer_name, current_layer["type"].GetString(), vector<map_object>{}, data);
        }


        if(current_layer["type"].GetString() == "objectgroup"s) {
            vector<map_object> objects;
            objects.resize(width*height);

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
                if(current_layer_enum == map_layer_name::Spawners && object_script_property != end(object_properties)) {
                    auto cache_it = spawner_script_cache.find(get<string>(object_script_property->value));
                    if(cache_it == end(spawner_script_cache)) {
                        spawn_script = get_spawner_script(get<string>(object_script_property->value), x, y, registry);
                        spawner_script_cache[get<string>(object_script_property->value)] = spawn_script;
                    } else {
                        spawn_script = cache_it->second;
                        spawn_script->loc = make_tuple(x, y);
                    }

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

                if(current_layer_enum == map_layer_name::NPCs) {
                    auto test = map_tilesets | ranges::views::remove_if([&](map_tileset const &t){ return t.firstgid > current_object["gid"].GetUint(); });

                    spawn_script = create_spawner_script_for_npc(current_object, x, y, test.back().firstgid);
                    spdlog::trace("[{}] npc {} assigned spawner", __FUNCTION__, current_object["name"].GetString());
                }

                objects[x + y * width] = map_object(gid, current_object["id"].GetUint(), current_object["width"].GetUint(),
                        current_object["height"].GetUint(), current_object["name"].GetString(), current_object["type"].GetString(), object_properties, spawn_script);
            }
            spdlog::trace("[{}] layer width {} height {} object size {}", __FUNCTION__, width, height, objects.size());
            // for some reason, width/height are not present in the json files?
            map_layers[current_layer_enum] = map_layer(current_layer["x"].GetInt(), current_layer["y"].GetInt(), width, height, current_layer_name, current_layer["type"].GetString(), move(objects), vector<uint32_t>{});
        }
    }

    vector<map_property> map_properties = get_properties(d["properties"]);

    spdlog::trace("[{}] map {} {} {}", __FUNCTION__, width, height, map_name);
    return make_optional<map_component>(width, height, move(map_name), move(map_properties), move(map_layers), move(map_tilesets));
}

