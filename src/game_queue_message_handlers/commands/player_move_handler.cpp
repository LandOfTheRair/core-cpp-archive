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

#include "player_move_handler.h"

#include <spdlog/spdlog.h>
#include <ecs/components.h>
#include <range/v3/view/filter.hpp>
#include <messages/generic_error_response.h>
#include <game_logic/logic_helpers.h>

using namespace std;

namespace lotr {
    void handle_player_move_message(queue_message* msg, entt::registry& registry, outward_queues& outward_queue) {
        auto *move_msg = dynamic_cast<player_move_message*>(msg);

        if(move_msg == nullptr) {
            spdlog::error("[{}] player_move_message nullptr", __FUNCTION__);
            return;
        }

        spdlog::trace("[{}] {} {} {}", __FUNCTION__, move_msg->x, move_msg->y, move_msg->connection_id);

        auto map_view = registry.view<map_component>();

        for(auto m_entity : map_view) {
            map_component &m = map_view.get(m_entity);
            auto player = m.players | ranges::views::remove_if([&](pc_component const &pc){ return pc.connection_id != move_msg->connection_id; });

            if(player.empty()) {
                continue;
            }

            if(move_msg->x >= m.width || move_msg->y >= m.height || !tile_is_walkable(m, move_msg->x, move_msg->y)) {
                spdlog::error("[{}] wrong coordinates {} {} {} {}", __FUNCTION__, m.name, move_msg->x, move_msg->y, move_msg->connection_id);
                outward_queue[move_msg->connection_id].enqueue(make_unique<generic_error_response>("Wrong coordinates", "Wrong coordinates", "Wrong coordinates", true));
                return;
            }

            player.back().loc = make_tuple(move_msg->x, move_msg->y);

            spdlog::info("[{}] {} db_character {} moved to {} {}", __FUNCTION__, move_msg->connection_id, player.back().name, move_msg->x, move_msg->y);
            break;
        }
    }
}
