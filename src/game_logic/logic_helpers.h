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


#pragma once

#include <entt/entt.hpp>
#include <ecs/components.h>

using namespace std;

namespace lotr {
    optional<npc_component> create_npc(global_npc_component const &global_npc, map_component const &m, spawner_script *script);
    void remove_dead_npcs(vector<npc_component> &npcs);
    void fill_spawners(map_component const &m, vector<npc_component> &npcs, entt::registry &registry);

    extern atomic<uint64_t> npc_id_counter;
}
