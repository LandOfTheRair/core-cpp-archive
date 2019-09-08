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

optional<npc_component> lotr::create_npc(global_npc_component const &global_npc, spawner_script *script) {
    if(global_npc.sprite.empty()) {
        spdlog::error("global npc {}:{} has no sprites", global_npc.name, global_npc.npc_id);
        return {};
    }

    npc_component npc;
    npc.id = npc_id_counter++;
    npc.name = global_npc.name;
    npc.npc_id = global_npc.npc_id;
    npc.allegiance = global_npc.allegiance;
    npc.alignment = global_npc.alignment;
    npc.sex = global_npc.sex;
    npc.dir = global_npc.dir;
    npc.hostility = global_npc.hostility;
    npc.character_class = global_npc.character_class;
    npc.monster_class = global_npc.monster_class;
    npc.spawn_message = global_npc.spawn_message;
    npc.sfx = global_npc.sfx;
    npc.level = npc.highest_level = global_npc.level;

    npc.sprite = global_npc.sprite[lotr::random.generate_single(0, global_npc.sprite.size() - 1)];
    npc.skill_on_kill = global_npc.skill_on_kill;
    npc.sfx_max_chance = global_npc.sfx_max_chance;

    npc.spawner = script;

    for (auto const &stat : stats) {
        auto const random_stat_it = find_if(cbegin(global_npc.random_stats), cend(global_npc.random_stats),
                                            [&](random_stat_component const &rs) noexcept { return rs.name == stat; });

        if (random_stat_it != cend(global_npc.random_stats)) {
            npc.stats[stat] = lotr::random.generate_single(random_stat_it->min, random_stat_it->max);
        } else {
            auto const stat_it = find_if(cbegin(global_npc.stats), cend(global_npc.stats),
                                         [&](stat_component const &s) noexcept { return s.name == stat; });
            if (stat_it == cend(global_npc.stats)) {
                spdlog::error("Initializing spawn for global_npc {} failed, missing stat {}", global_npc.npc_id, stat);
                return {};
            }
            npc.stats[stat] = stat_it->value;
        }

        spdlog::trace("Added {}:{} to npc {}:{}", stat, npc.stats[stat], npc.id, npc.name);
    }

    spdlog::debug("Created npc {}:{}", npc.name, npc.npc_id);
    return npc;
}

void lotr::remove_dead_npcs(vector<npc_component> &npcs) {
    if(!npcs.empty()) {
        remove_if(begin(npcs), end(npcs), [&](npc_component &npc) noexcept { return npc.stats["hp"] <= 0; });
    }
}

void lotr::fill_spawners(vector<npc_component> &npcs, entt::registry &registry) {
    lotr_flat_map<uint32_t, tuple<uint32_t, spawner_script*>> spawner_npc_counter;

    for(auto &npc : npcs) {
        if(npc.stats["hp"] <= 0) {
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

            auto random_npc_id = get<1>(v)->npc_ids[lotr::random.generate_single(0, get<1>(v)->npc_ids.size() - 1)].name;

            registry.view<global_npc_component>().each([&](global_npc_component const &global_npc) noexcept {
                if(global_npc.npc_id != random_npc_id) {
                    return;
                }

                auto npc = create_npc(global_npc, get<1>(v));

                if(npc) {
                    npcs.emplace_back(move(*npc));
                }
            });
        }
    }
}
