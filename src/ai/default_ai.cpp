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

using namespace lotr;

vector<pc_component*> get_players_in_range(character_component const &npc, map_component const &m, lotr_player_location_map const &player_location_map, int32_t radius) {
    vector<pc_component*> ret;

    for(int32_t x_radius = -radius; x_radius < radius; x_radius++) {
        if(static_cast<int32_t>(npc.x) - x_radius < 0 || static_cast<int32_t>(npc.x) - x_radius >= static_cast<int32_t>(m.width)) {
            continue;
        }

        for(int32_t y_radius = -radius; y_radius < radius; y_radius++) {
            if(static_cast<int32_t>(npc.y) - y_radius < 0 || static_cast<int32_t>(npc.y) - y_radius >= static_cast<int32_t>(m.height)) {
                continue;
            }

            auto it = player_location_map.find(make_tuple(npc.x - x_radius, npc.y - y_radius));
            if(it != end(player_location_map)) {
                ret.insert(end(ret), begin(it->second), end(it->second));
            }
        }
    }

    return ret;
}

void check_ground_for_items(npc_component &npc, map_component &m) {
    if(npc.items.find("right_hand") != end(npc.items) && npc.items.find("right_hand") != end(npc.items)) {
        return;
    }
}

void lotr::run_ai_on(npc_component &npc, map_component &m, lotr_player_location_map const &player_location_map) {
    if(npc.hostility == "never"s) {
        return;
    }

    pc_component *current_target = nullptr;
    bool player_on_same_location = player_location_map.find(make_tuple(npc.x, npc.y)) != end(player_location_map);
    if(!(npc.hostility == "OnHit" && npc.agro_target == nullptr) && player_on_same_location) {
        auto targets_in_range = get_players_in_range(npc, m, player_location_map, 4);

        if(!targets_in_range.empty()) {
            current_target = targets_in_range[lotr::random.generate_single(0, targets_in_range.size() - 1)];
        }
    }

    auto move_rate = npc.stats[stat_move];
    auto num_steps = 0;
    if(move_rate > 0) {
        num_steps = lotr::random.generate_single(0, move_rate);
    }

    if(lotr::random.one_in_x(100)) {
        check_ground_for_items(npc, m);
    }

    if(current_target != nullptr) {

    } else if(num_steps > 0) {

    }
}
