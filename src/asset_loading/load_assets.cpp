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

#include "load_assets.h"
#include <filesystem>
#include "spdlog/spdlog.h"
#include "load_item.h"
#include "load_npc.h"
#include "load_map.h"
#include <random_helper.h>
#include <game_logic/logic_helpers.h>

using namespace std;
using namespace lotr;

#define NPC_STRING_FIELD(x) if(npc_node[ #x ]) { npc. x = npc_node[ #x ].as<string>(); }
#define NPC_UINT_FIELD(x) if(npc_node[ #x ]) { npc. x = npc_node[ #x ].as<uint32_t>(); }
#define NPC_BOOL_FIELD(x) if(npc_node[ #x ]) { npc. x = npc_node[ #x ].as<bool>(); }

void lotr::load_assets(entt::registry &registry, atomic<bool> &quit) {
    uint32_t item_count = 0;
    uint32_t npc_count = 0;
    uint32_t map_count = 0;
    uint32_t entity_count = 0;
    auto items_loading_start = chrono::system_clock::now();
    for(auto& p: filesystem::recursive_directory_iterator("assets/items")) {
        if(!p.is_regular_file() || quit) {
            continue;
        }

        auto items = load_global_items_from_file(p.path().string());

        for(auto &item: items) {
            auto new_entity = registry.create();
            registry.assign<global_item_component>(new_entity, move(item));
            item_count++;
        }
    }

    auto npcs_loading_start = chrono::system_clock::now();
    for(auto& p: filesystem::recursive_directory_iterator("assets/npcs")) {
        if(!p.is_regular_file() || quit) {
            continue;
        }

        auto npcs = load_global_npcs_from_file(p.path().string());

        for(auto &npc: npcs) {
            auto new_entity = registry.create();
            registry.assign<global_npc_component>(new_entity, move(npc));
            npc_count++;
        }
    }

    auto maps_loading_start = chrono::system_clock::now();
    for(auto& p: filesystem::recursive_directory_iterator("assets/maps")) {
        if(!p.is_regular_file() || quit) {
            continue;
        }

        auto map = load_map_from_file(p.path().string(), registry);

        if(!map) {
            continue;
        }

        auto const npcs_layer = find_if(cbegin(map->layers), cend(map->layers), [](map_layer const &l) noexcept { return l.name == npcs_layer_name; }); // Case-sensitive. This will probably bite us in the ass later.
        if(npcs_layer != cend(map->layers)) {
            for(auto &npc : npcs_layer->objects) {
                if(npc.gid == 0 || !npc.script) {
                    continue;
                }

                if(npc.script->initial_spawn == 0 || npc.script->max_creatures == 0 || npc.script->npc_ids.empty()) {
                    continue;
                }

                auto new_entity = registry.create();
                spawner_script new_spawner;
                global_npc_component gnpc{};

                gnpc.name = npc.name;
                gnpc.npc_id = npc.name;
                gnpc.sprite.push_back(npc.gid - map->tilesets[3].firstgid);

                for(auto &stat : stats) {
                    gnpc.stats.emplace_back(stat, 10);
                }

                registry.assign<global_npc_component>(new_entity, move(gnpc));
                npc_count++;

                spdlog::trace("[{}] npc {} assigned global npc component", __FUNCTION__, npc.name);
            }
        } else {
            spdlog::error("[{}] no npcs layer found", __FUNCTION__);
        }

        auto new_entity = registry.create();
        registry.assign<map_component>(new_entity, move(map.value()));
        map_count++;
    }

    auto entity_spawning_start = chrono::system_clock::now();
    registry.view<map_component>().each([&](map_component &m) noexcept {
        if(quit) {
            return;
        }

        auto spawners_layer = find_if(begin(m.layers), end(m.layers), [&](map_layer const &l) noexcept {return l.name == spawners_layer_name;}); // Case-sensitive. This will probably bite us in the ass later.
        if(spawners_layer == end(m.layers)) {
            spdlog::warn("No spawner layer found for map {}", m.name);
            return;
        }

        for(auto &spawner_object : spawners_layer->objects) {
            if(spawner_object.gid == 0 || !spawner_object.script) {
                continue;
            }

            if(spawner_object.script->initial_spawn == 0 || spawner_object.script->max_creatures == 0 || spawner_object.script->npc_ids.empty()) {
                continue;
            }

            for(uint32_t i = 0; i < spawner_object.script->initial_spawn; i++) {
                spdlog::info("spawner_object {} has {} npc_ids", spawner_object.name, spawner_object.script->npc_ids.size());
                auto &random_npc_id = spawner_object.script->npc_ids[lotr::random.generate_single(0, spawner_object.script->npc_ids.size() - 1)];

                auto npc = create_npc(random_npc_id, m, &spawner_object.script.value());

                if(npc) {
                    m.npcs.emplace_back(move(*npc));
                    entity_count++;
                }
            }
        }
    });

    registry.view<map_component>().each([&](map_component &m) noexcept {
        if(quit) {
            return;
        }

        auto npcs_layer = find_if(begin(m.layers), end(m.layers), [&](map_layer const &l) noexcept {return l.name == npcs_layer_name;}); // Case-sensitive. This will probably bite us in the ass later.
        if(npcs_layer == end(m.layers)) {
            spdlog::warn("No npc layer found for map {}", m.name);
            return;
        }

        for(auto &npc_object : npcs_layer->objects) {
            if(npc_object.gid == 0 || !npc_object.script) {
                continue;
            }

            if(npc_object.script->initial_spawn == 0 || npc_object.script->max_creatures == 0 || npc_object.script->npc_ids.empty()) {
                continue;
            }

            auto npc = create_npc(npc_object.script->npc_ids[0], m, &npc_object.script.value());

            if(npc) {
                m.npcs.emplace_back(move(*npc));
                entity_count++;
            }
        }
    });

    auto loading_end = chrono::system_clock::now();
    spdlog::info("[{}] {} items loaded in {} µs", __FUNCTION__, item_count, chrono::duration_cast<chrono::microseconds>(npcs_loading_start - items_loading_start).count());
    spdlog::info("[{}] {} npcs loaded in {} µs", __FUNCTION__, npc_count, chrono::duration_cast<chrono::microseconds>(maps_loading_start - npcs_loading_start).count());
    spdlog::info("[{}] {} maps loaded in {} µs", __FUNCTION__, map_count, chrono::duration_cast<chrono::microseconds>(entity_spawning_start - maps_loading_start).count());
    spdlog::info("[{}] {} entities spawned in {} µs", __FUNCTION__, entity_count, chrono::duration_cast<chrono::microseconds>(loading_end - entity_spawning_start).count());
    spdlog::info("[{}] everything loaded in {} µs", __FUNCTION__, chrono::duration_cast<chrono::microseconds>(loading_end - items_loading_start).count());
}

