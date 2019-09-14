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

#pragma once

#include <string>
#include "../ecs/components.h"

using namespace std;

namespace lotr {
    using lotr_player_location_map = lotr_flat_map <tuple<uint64_t, uint64_t>, vector<pc_component*>>;

    void run_ai_on(npc_component &npc, map_component &m, lotr_player_location_map const &player_location_map);
}
