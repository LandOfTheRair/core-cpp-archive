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

#include <bitset>

using namespace std;

namespace lotr {
    constexpr uint32_t fov_max_distance = 4;
    constexpr uint32_t fov_diameter = fov_max_distance * 2 + 1;

    constexpr uint32_t power(uint32_t x) noexcept {
        return x * x;
    }

    struct map_component;

    [[nodiscard]]
    bitset<power(fov_diameter)> compute_fov_restrictive_shadowcasting(map_component const &m, int32_t const player_x, int32_t const player_y, bool const light_walls);

    void log_fov(bitset<power(fov_diameter)> const &fov, string name);
}
