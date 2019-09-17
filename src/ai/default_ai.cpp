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

#include "default_ai.h"

#include <spdlog/spdlog.h>
#include <random_helper.h>
#include <game_logic/a_star.h>
#include <range/v3/view/reverse.hpp>
#include <game_logic/logic_helpers.h>

using namespace std;
using namespace lotr;

[[nodiscard]]
vector<pc_component*> get_players_in_range(character_component const &npc, map_component const &m, lotr_player_location_map const &player_location_map, int32_t radius) {
    vector<pc_component*> ret;

    for(int32_t x_radius = -radius; x_radius <= radius; x_radius++) {
        if(static_cast<int32_t>(get<0>(npc.loc)) - x_radius < 0 || static_cast<int32_t>(get<0>(npc.loc)) - x_radius >= static_cast<int32_t>(m.width)) {
            continue;
        }

        for(int32_t y_radius = -radius; y_radius <= radius; y_radius++) {
            if(static_cast<int32_t>(get<1>(npc.loc)) - y_radius < 0 || static_cast<int32_t>(get<1>(npc.loc)) - y_radius >= static_cast<int32_t>(m.height)) {
                continue;
            }

            auto it = player_location_map.find(make_tuple(get<0>(npc.loc) - x_radius, get<1>(npc.loc) - y_radius));
            if(it != end(player_location_map)) {
                ret.insert(end(ret), begin(it->second), end(it->second));
            }
        }
    }

    return ret;
}

void check_ground_for_items(npc_component &npc, map_component &m) {
    if(npc.items.find(gear_slot_right_hand) != end(npc.items) && npc.items.find(gear_slot_left_hand) != end(npc.items)) {
        return;
    }


}

[[nodiscard]]
uint32_t distance_between(location a, location b) {
    return sqrt(pow(get<0>(b) - get<0>(a), 2) + pow(get<1>(b) - get<1>(a), 2));
}

[[nodiscard]]
tuple<int32_t, int32_t> direction_to_position_offset(movement_direction direction, int32_t steps) {
    switch(direction) {
        case movement_direction::North:
            return make_tuple(0, steps);
        case movement_direction::East:
            return make_tuple(steps, 0);
        case movement_direction::South:
            return make_tuple(0, -steps);
        case movement_direction::West:
            return make_tuple(-steps, 0);
        case movement_direction::NorthEast:
            return make_tuple(steps, steps);
        case movement_direction::NorthWest:
            return make_tuple(steps, -steps);
        case movement_direction::SouthEast:
            return make_tuple(-steps, steps);
        case movement_direction::SouthWest:
            return make_tuple(-steps, -steps);
    }
    spdlog::error("[{}] code path should never be hit", __FUNCTION__);
    return make_tuple(0, 0);
}


void move_npc_along_path(npc_component &npc, int num_steps) {
    auto offset = direction_to_position_offset(npc.paths[npc.current_path_index].direction, num_steps);
    get<0>(npc.loc) += get<0>(offset);
    get<1>(npc.loc) += get<1>(offset);
    npc.steps_remaining_in_path -= num_steps;

    if(npc.steps_remaining_in_path == 0) {
        npc.current_path_index++;
        if(npc.current_path_index >= npc.paths.size()) {
            npc.current_path_index = 0;
        }
        npc.steps_remaining_in_path = npc.paths[npc.current_path_index].steps;
    }
}

void lotr::run_ai_on(npc_component &npc, map_component &m, lotr_player_location_map const &player_location_map) {
    if(npc.hostility == hostility_never) {
        return;
    }

    pc_component *current_target = nullptr;
    bool player_on_same_location = player_location_map.find(make_tuple(get<0>(npc.loc), get<1>(npc.loc))) != end(player_location_map);
    if(!(npc.hostility == hostility_on_hit && npc.agro_target == nullptr) && player_on_same_location) {
        auto targets_in_range = get_players_in_range(npc, m, player_location_map, 4);

        if(!targets_in_range.empty()) {
            current_target = targets_in_range.size() > 1 ? targets_in_range[lotr::random.generate_single_fast(targets_in_range.size() - 1)] : 0;
        }
    }

    auto move_rate = npc.stats[stat_move];
    auto num_steps = 0;
    if(npc.paths.empty()) {
        num_steps = lotr::random.generate_single_fast(move_rate);
    } else {
        num_steps = lotr::random.generate_single_fast(min(move_rate, npc.steps_remaining_in_path));
    }

    if(lotr::random.one_in_x(100)) {
        check_ground_for_items(npc, m);
    }

    if(current_target != nullptr) {

    } else if(!npc.paths.empty()) {
        if(npc.is_path_interrupted) {
            if(get<0>(npc.loc) == get<0>(npc.loc_before_interruption) && get<1>(npc.loc) == get<1>(npc.loc_before_interruption)) {
                npc.is_path_interrupted = false;
                move_npc_along_path(npc, num_steps);
            } else {
                auto paths = a_star_path(m, npc.loc, npc.loc_before_interruption);
                vector<location> path;
                path.reserve(fov_max_distance);

                {
                    auto &next = paths[npc.loc_before_interruption];
                    while (next != npc.loc) {
                        path.push_back(next);
                        next = paths[next];
                    }
                }

                int32_t steps = 0;
                for (auto& next : path | ranges::views::reverse)
                {
                    if(steps < num_steps) {
                        steps++;
                        continue;
                    }

                    npc.loc = next;

                    break;
                }
            }
        } else {
            move_npc_along_path(npc, num_steps);
        }
    } else {
        //wander
        while(num_steps > 0) {
            auto x = lotr::random.generate_single(-1l, 1l);
            auto y = lotr::random.generate_single(-1l, 1l);

            if(tile_is_walkable(m.layers[map_layer_name::Walls], m.layers[map_layer_name::OpaqueDecor], get<0>(npc.loc) + x, get<1>(npc.loc) + y)) {
                npc.loc = make_tuple(get<0>(npc.loc) + x, get<1>(npc.loc) + y);
                num_steps--;
            }
        }
    }

    auto distance_from_spawner = distance_between(npc.loc, npc.spawner->loc);
    if(npc.paths.empty() && ((current_target == nullptr && distance_from_spawner > npc.spawner->random_walk_radius) ||
        distance_from_spawner > npc.spawner->leash_radius)) {
        // send leash message

        npc.loc = npc.spawner->loc;

        if(distance_from_spawner > npc.spawner->leash_radius + 4) {
            npc.stats[stat_hp] = npc.stats[stat_max_hp];
            npc.stats[stat_mp] = npc.stats[stat_max_mp];
        }
    }
}
