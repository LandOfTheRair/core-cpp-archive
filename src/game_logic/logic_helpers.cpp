/*
    Land of the Rair
    Copyright (C) 2019 Michael de Lang

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

#include "logic_helpers.h"

#include <spdlog/spdlog.h>
#include <random_helper.h>

using namespace std;
using namespace lotr;

atomic<uint64_t> lotr::npc_id_counter;

optional<npc_component> lotr::create_npc(spawner_npc_id const &spawner_npc_id, map_component const &m, spawner_script *script) {
    if(spawner_npc_id.sprite.empty()) {
        spdlog::error("spawner npc {} has no sprites", spawner_npc_id.npc_id);
        return {};
    }

    npc_component npc;
    npc.id = npc_id_counter++;
    npc.npc_id = spawner_npc_id.npc_id;
    npc.allegiance = spawner_npc_id.allegiance;
    npc.alignment = spawner_npc_id.alignment;
    npc.sex = spawner_npc_id.sex;
    npc.dir = spawner_npc_id.dir;
    npc.hostility = spawner_npc_id.hostility;
    npc.character_class = spawner_npc_id.character_class;
    npc.monster_class = spawner_npc_id.monster_class;
    npc.spawn_message = spawner_npc_id.spawn_message;
    npc.sfx = spawner_npc_id.sfx;
    npc.level = npc.highest_level = spawner_npc_id.level;

    npc.sprite = spawner_npc_id.sprite[lotr::random.generate_single(0, spawner_npc_id.sprite.size() - 1)];
    npc.skill_on_kill = spawner_npc_id.skill_on_kill;
    npc.sfx_max_chance = spawner_npc_id.sfx_max_chance;

    npc.spawner = script;

    if(script->spawn_radius > 0) {
        bool found_coord = false;
        auto const walls_layer = find_if(cbegin(m.layers), cend(m.layers), [](map_layer const &l) noexcept {return l.name == wall_layer_name;}); // Case-sensitive. This will probably bite us in the ass later.
        auto const opaque_layer = find_if(cbegin(m.layers), cend(m.layers), [](map_layer const &l) noexcept {return l.name == opaque_layer_name;}); // Case-sensitive. This will probably bite us in the ass later.
        while(!found_coord) {
            npc.x = lotr::random.generate_single((uint64_t)script->x - script->spawn_radius, script->x + script->spawn_radius);
            npc.y = lotr::random.generate_single((uint64_t)script->y - script->spawn_radius, script->y + script->spawn_radius);
            int32_t c = npc.x + (npc.y * m.width);

            spdlog::trace("[{}] c {} npc x {} y {} w {} h {} map {} script x {} script y {} spawn_radius {} wall {} object {}",  __FUNCTION__, c, npc.x, npc.y, m.width, m.height, m.name, script->x, script->y, script->spawn_radius, walls_layer->data[c], opaque_layer->objects[c].gid);
            if(walls_layer->data[c] == 0 && opaque_layer->objects[c].gid == 0) {
                found_coord = true;
            }
        }
    } else {
        npc.x = script->x;
        npc.y = script->y;
    }

    for (auto const &stat : stats) {
        auto const random_stat_it = find_if(cbegin(spawner_npc_id.random_stats), cend(spawner_npc_id.random_stats),
                                            [&](random_stat_component const &rs) noexcept { return rs.name == stat; });

        if (random_stat_it != cend(spawner_npc_id.random_stats)) {
            npc.stats[stat] = lotr::random.generate_single(random_stat_it->min, random_stat_it->max);
        } else {
            auto const stat_it = find_if(cbegin(spawner_npc_id.stats), cend(spawner_npc_id.stats),
                                         [&](stat_component const &s) noexcept { return s.name == stat; });
            if (stat_it == cend(spawner_npc_id.stats)) {
                spdlog::error("[{}] Initializing spawn for spawner_npc_id {} failed, missing stat {}", __FUNCTION__, spawner_npc_id.npc_id, stat);
                return {};
            }
            npc.stats[stat] = stat_it->value;
        }

        //spdlog::trace("[{}] Added {}:{} to npc {}:{}", __FUNCTION__, stat, npc.stats[stat], npc.id, npc.name);
    }

    spdlog::debug("[{}] Created npc {}:{}", __FUNCTION__, npc.name, npc.npc_id);
    return npc;
}

void lotr::remove_dead_npcs(vector<npc_component> &npcs) noexcept {
    npcs.erase(remove_if(begin(npcs), end(npcs), [&](npc_component &npc) noexcept { return npc.stats[stat_hp] <= 0; }), end(npcs));
}

void lotr::fill_spawners(map_component const &m, vector<npc_component> &npcs, entt::registry &registry) {
    lotr_flat_map<uint32_t, tuple<uint32_t, spawner_script*>> spawner_npc_counter;

    for(auto &npc : npcs) {
        if(npc.stats[stat_hp] <= 0) {
            continue;
        }

        if(npc.spawner) {
            auto spawner_it = spawner_npc_counter.find(npc.spawner->id);

            if (spawner_it == end(spawner_npc_counter)) {
                get<0>(spawner_it->second)++;
            } else {
                spawner_npc_counter[npc.spawner->id] = make_tuple(1, npc.spawner);
            }
        }
    }

    for(auto &[k, v] : spawner_npc_counter) {
        if(get<0>(v) < get<1>(v)->max_creatures) {
            if(get<1>(v)->npc_ids.empty()) {
                continue;
            }

            auto &random_npc_id = get<1>(v)->npc_ids[lotr::random.generate_single(0, get<1>(v)->npc_ids.size() - 1)];

            auto npc = create_npc(random_npc_id, m, get<1>(v));

            if(npc) {
                npcs.emplace_back(move(*npc));
            }
        }
    }
}
