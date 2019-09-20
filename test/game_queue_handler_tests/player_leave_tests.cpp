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

#include <catch2/catch.hpp>
#include "../test_helpers/startup_helper.h"
#include <game_queue_message_handlers/player_leave_handler.h>
#include <ecs/components.h>

using namespace std;
using namespace lotr;

TEST_CASE("db_character leave tests") {
    SECTION( "db_character leaves world" ) {
        entt::registry registry;

        auto new_entity = registry.create();
        {
            map_component test_map(10, 10, "test", {}, {}, {});
            pc_component pc;
            pc.connection_id = 1;
            pc.name = "test_player";
            test_map.players.emplace_back(move(pc));

            registry.assign<map_component>(new_entity, move(test_map));
        }

        outward_queues q;
        player_leave_message msg(1);
        handle_player_leave_message(&msg, registry, q);

        auto &test_map = registry.get<map_component>(new_entity);

        REQUIRE(test_map.players.size() == 0);
    }
}
