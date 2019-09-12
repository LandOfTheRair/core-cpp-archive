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

#include "player_leave_message_handler.h"

#include <spdlog/spdlog.h>
#include <ecs/components.h>

using namespace std;

namespace lotr {
    void handle_player_leave_message(queue_message* msg, entt::registry& registry) {
        auto *leave_message = dynamic_cast<player_leave_message*>(msg);

        if(leave_message == nullptr) {
            spdlog::error("[{}] leave_message nullptr", __FUNCTION__);
            return;
        }

        auto map_view = registry.view<map_component>();

        for(auto m_entity : map_view) {
            map_component &m = map_view.get(m_entity);
            m.players.erase(remove_if(begin(m.players), end(m.players), [&](pc_component const &pc) { return pc.name == leave_message->character_name; }), end(m.players));
        }

        spdlog::info("[{}] player {} left game", __FUNCTION__, leave_message->character_name);
    }
}
