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
#include <spdlog/spdlog.h>

using namespace std;

namespace lotr {
    optional<npc_component> create_npc(spawner_npc_id const &spawner_npc_id, map_component const &m, spawner_script *script);
    void remove_dead_npcs(vector<npc_component> &npcs) noexcept;
    void fill_spawners(map_component const &m, vector<npc_component> &npcs, entt::registry &registry);

    static bool tile_is_walkable(map_component const &m, int32_t const x, int32_t const y) {
        auto const &walls_layer = m.layers[map_layer_name::Walls];
        auto const &opaque_layer = m.layers[map_layer_name::OpaqueDecor];

        if(x < 0 || x >= walls_layer.width || y < 0 || y >= walls_layer.height){
            return false;
        }

        uint32_t c = x + y * walls_layer.width;

#ifndef NDEBUG
        if(c >= walls_layer.data.size()) {
            spdlog::error("[{}] c larger than walls for map {} {}: {} {}, {} {} {} {}", __FUNCTION__, m.name, walls_layer.name, c, walls_layer.data.size(), x, y, walls_layer.width, walls_layer.height);
        }
        if(c >= opaque_layer.objects.size()) {
            spdlog::error("[{}] c larger than opaque for map {} {}: {} {}, {} {} {} {}", __FUNCTION__, m.name, opaque_layer.name, c, opaque_layer.objects.size(), x, y, opaque_layer.width, opaque_layer.height);
        }
#endif

        return walls_layer.data[c] == 0 && opaque_layer.objects[c].gid == 0;
    }

    inline bool tile_is_walkable(map_component const &m, location const &loc) {
        return tile_is_walkable(m, get<0>(loc), get<1>(loc));
    }
    
    inline bool is_visible(location const main_entity, location const other, bitset<power(fov_diameter)> const &fov, int32_t const min_x, int32_t const max_x, int32_t const min_y, int32_t const max_y) {
        return get<0>(other) >= min_x && get<0>(other) <= max_x &&
            get<1>(other) >= min_y && get<1>(other) <= max_y &&
            fov[get<0>(main_entity) - get<0>(other) + fov_max_distance + ((get<1>(other) - get<1>(main_entity) + fov_max_distance) * fov_diameter)] == true;
    }

    extern atomic<uint64_t> npc_id_counter;
}
