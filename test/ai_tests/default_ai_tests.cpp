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
#include <ai/default_ai.h>

using namespace std;
using namespace lotr;

vector<pc_component*> get_players_in_range(character_component const &npc, map_component const &m, lotr_player_location_map const &player_location_map, int32_t radius);

TEST_CASE("default ai tests") {
    lotr_player_location_map locations;
    pc_component pc;
    npc_component npc;
    npc.x = 3;
    npc.y = 3;
    map_component m(10, 10, "test", {}, {}, {});
    locations[make_tuple(1,1)].push_back(&pc);

    auto pcs_in_range = get_players_in_range(npc, m, locations, 1);

    REQUIRE(pcs_in_range.empty());

    pcs_in_range = get_players_in_range(npc, m, locations, 2);

    REQUIRE(pcs_in_range.size() == 1);
}
