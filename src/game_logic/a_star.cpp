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

#include "a_star.h"
#include <spdlog/spdlog.h>
#include <queue>

using namespace lotr;

//#define EXTREME_A_STAR_LOGGING

template <class T>
vector<T> make_reserved(size_t const n)
{
    vector<T> v;
    v.reserve(n);
    return v;
}

struct a_star_priority_queue {
    using priority_element = pair<int, location>;

    priority_queue<priority_element, vector<priority_element>, greater<priority_element>> frontier_queue;

    a_star_priority_queue(size_t const reserved_size) : frontier_queue(greater<priority_element>(), make_reserved<priority_element>(reserved_size)) {}

    inline bool empty() const {
        return frontier_queue.empty();
    }

    inline void put(location item, int priority) {
        frontier_queue.emplace(priority, item);
    }

    inline location get() {
        location best_item = frontier_queue.top().second;
        frontier_queue.pop();
        return best_item;
    }
};

uint32_t heuristic(location const &a, location const &b) noexcept {
    return abs(get<0>(a) - get<0>(b)) + abs(get<1>(a) - get<1>(b));
}

array<location, 9> get_neighbours(map_component const &m, map_layer const *walls_layer, map_layer const *opaque_layer, location const &loc) {
    array<location, 9> locs{location{-1, -1}, location{-1, -1}, location{-1, -1}, location{-1, -1}, location{-1, -1}, location{-1, -1}, location{-1, -1}, location{-1, -1}, location{-1, -1}};
    uint32_t counter = 0;

    for(int32_t x = max(get<0>(loc) - 1, 0); x <= min(get<0>(loc) + 1, static_cast<int32_t >(m.width)); x++) {
        for(int32_t y = max(get<1>(loc) - 1, 0); y <= min(get<1>(loc) + 1, static_cast<int32_t >(m.height)); y++) {
#ifdef EXTREME_A_STAR_LOGGING
            spdlog::warn("[{}] {}:{}", __FUNCTION__, x, y);
#endif
            uint32_t c = x + y * walls_layer->width;
            if(walls_layer->data[c] == 0 && opaque_layer->objects[c].gid == 0) {
                locs[counter] = make_tuple(x, y);
                counter++;
            }
        }
    }

    return locs;
}

lotr_flat_map<location, location> lotr::a_star_path(map_component const &m, location const &start, location const &goal) {
    uint32_t reserve_size = min(m.width, 8u) * min(m.height, 8u);
    a_star_priority_queue frontier(reserve_size);
    lotr_flat_map<location, location> came_from;
    lotr_flat_map<location, uint32_t> cost_so_far;
    auto const walls_layer = find_if(cbegin(m.layers), cend(m.layers), [](map_layer const &l) noexcept {return l.name == wall_layer_name;}); // Case-sensitive. This will probably bite us in the ass later.
    auto const opaque_layer = find_if(cbegin(m.layers), cend(m.layers), [](map_layer const &l) noexcept {return l.name == opaque_layer_name;}); // Case-sensitive. This will probably bite us in the ass later.

    came_from.reserve(reserve_size);
    cost_so_far.reserve(reserve_size);
    cost_so_far[start] = 0;
    frontier.put(start, 0);
    while(!frontier.empty()) {
        location current = frontier.get();
#ifdef EXTREME_A_STAR_LOGGING
        spdlog::warn("[{}] current {}:{}", __FUNCTION__, get<0>(current), get<1>(current));
#endif

        if(current == goal) {
            break;
        }

        for (auto next : get_neighbours(m, &*walls_layer, &*opaque_layer, current)) {
            if(get<0>(next) == -1) {
                break;
            }

            auto cost_so_far_it = cost_so_far.find(next);
            uint32_t new_cost = cost_so_far_it == end(cost_so_far) ? 1 : cost_so_far_it->second + 1;
#ifdef EXTREME_A_STAR_LOGGING
            spdlog::warn("[{}] next {}:{}:{}:{}", __FUNCTION__, get<0>(next), get<1>(next), cost_so_far_it == end(cost_so_far), new_cost);
#endif
            if (cost_so_far_it == end(cost_so_far) || new_cost < cost_so_far_it->second) {
                cost_so_far[next] = new_cost;
                uint32_t priority = new_cost + heuristic(next, goal);
                frontier.put(next, priority);
                came_from[next] = current;
            }
        }
    }

#ifdef EXTREME_A_STAR_LOGGING
    spdlog::warn("[{}] came_from {}", __FUNCTION__, came_from.size());
#endif

    return came_from;
}
