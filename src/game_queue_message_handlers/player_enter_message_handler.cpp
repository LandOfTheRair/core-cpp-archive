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

#include "player_enter_message_handler.h"

#include <spdlog/spdlog.h>
#include <ecs/components.h>

using namespace std;

namespace lotr {
    void handle_player_enter_message(queue_message* msg, entt::registry& registry) {
        auto *enter_msg = dynamic_cast<player_enter_message*>(msg);

        if(enter_msg == nullptr) {
            spdlog::error("[{}] enter_message nullptr", __FUNCTION__);
            return;
        }

        spdlog::trace("[{}] {} {} {}", __FUNCTION__, enter_msg->map_name, enter_msg->x, enter_msg->y);

        auto map_view = registry.view<map_component>();

        for(auto m_entity : map_view) {
            map_component &m = map_view.get(m_entity);

            if(m.name != enter_msg->map_name) {
                continue;
            }

            pc_component pc{};
            pc.name = enter_msg->character_name;
            pc.x = enter_msg->x;
            pc.y = enter_msg->y;
            pc.connection_id = enter_msg->connection_id;

            if(pc.x >= m.width || pc.y >= m.height) {
                spdlog::error("[{}] wrong coordinates {} {} {}", __FUNCTION__, enter_msg->map_name, enter_msg->x, enter_msg->y);
                break;
            }

            for(auto &stat : enter_msg->player_stats) {
                pc.stats[stat.name] = stat.value;
            }

            m.players.emplace_back(pc);

            spdlog::info("[{}] player {} entered game", __FUNCTION__, pc.name);
            break;
        }
    }
}
