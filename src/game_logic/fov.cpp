/* BSD 3-Clause License
 *
 * Copyright © 2008-2019, Jice and the libtcod contributors.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
* Mingos' Restrictive Precise Angle Shadowcasting (MRPAS) v1.2.
 *
 * Shamelessly taken from libtcod and edited accordingly.
*/

#include "fov.h"
#include <algorithm>
#include <bitset>
#include <ecs/components.h>
#include <memory>
#include <spdlog/spdlog.h>

//#define LOG_FOV_EXTREME 1

using namespace std;
using namespace lotr;

/* angle ranges */
unique_ptr<double[]> start_angle = nullptr;
unique_ptr<double[]> end_angle = nullptr;
/* number of allocated angle pairs */
int allocated = 0;

[[nodiscard]]
bitset<power(fov_diameter)> compute_fov_restrictive_shadowcasting_quadrant (map_component const &m, map_layer const *walls_layer, map_layer const *opaque_layer,
                                                                            int32_t const player_x, int32_t const player_y, bool const light_walls, int const dx, int const dy) {
    bitset<power(fov_diameter)> fov_array{};

    // players location is always visible
    fov_array[fov_max_distance + fov_max_distance * fov_diameter] = true;

    /* octant: vertical edge */
    {
        int32_t iteration = 1; /* iteration of the algo for this octant */
        bool done = false;
        int32_t total_obstacles = 0;
        int32_t obstacles_in_last_line = 0;
        double min_angle = 0.0;
        int32_t x;
        int32_t y;

        /* do while there are unblocked slopes left and the algo is within the map's boundaries
           scan progressive lines/columns from the PC outwards */
        y = player_y+dy; /* the outer slope's coordinates (first processed line) */
        if (y < 0 || y >= static_cast<int32_t>(m.height)) {
            done = true;
        }
        while (!done) {
            /* process cells in the line */
            double slopes_per_cell = 1.0 / (double)(iteration);
            double half_slopes = slopes_per_cell * 0.5;
            int32_t processed_cell = static_cast<int32_t>((min_angle + half_slopes) / slopes_per_cell);
            int32_t minx = max(0, player_x - iteration);
            int32_t maxx = min(static_cast<int32_t>(m.width) - 1, player_x + iteration);
            done = true;
            for (x = player_x + (processed_cell * dx); x >= minx && x <= maxx; x+=dx) {
                int32_t c = x + (y * m.width);
                int32_t c_fov = x - player_x + fov_max_distance + ((y - player_y + fov_max_distance) * fov_diameter);

                /* calculate slopes per cell */
                bool visible = true;
                bool extended = false;
                double centre_slope = (double)processed_cell * slopes_per_cell;
                double start_slope = centre_slope - half_slopes;
                double end_slope = centre_slope + half_slopes;
                auto const &object = opaque_layer->objects[c];

#ifdef LOG_FOV_EXTREME
                spdlog::trace("[{}] vertical {}:{} c {} wall {}", __FUNCTION__, x, y, c, walls_layer->data[c]);
#endif

                if (obstacles_in_last_line > 0) {
                    if (!(fov_array[c_fov - (fov_diameter * dy)] && walls_layer->data[c-(m.width * dy)] == 0 && opaque_layer->objects[c-(m.width * dy)].gid == 0) &&
                        !(fov_array[c_fov - (fov_diameter * dy) - dx] && walls_layer->data[c-(m.width * dy) - dx] == 0 && opaque_layer->objects[c-(m.width * dy) - dx].gid == 0))
                    {
                        visible = false;
                    } else {
                        for (int32_t idx = 0; idx < obstacles_in_last_line && visible; ++idx) {
                            if (start_slope <= end_angle[idx] && end_slope >= start_angle[idx]) {
                                if (walls_layer->data[c] != 0 || object.gid != 0) {
                                    if (centre_slope > start_angle[idx] && centre_slope < end_angle[idx]) {
                                        visible = false;
                                    }
                                } else {
                                    if (start_slope >= start_angle[idx] && end_slope <= end_angle[idx]) {
                                        visible = false;
                                    } else {
                                        start_angle[idx] = min(start_angle[idx], start_slope);
                                        end_angle[idx] = max(end_angle[idx], end_slope);
                                        extended = true;
                                    }
                                }
                            }
                        }
                    }
                }
                if (visible) {
                    done = false;

                    fov_array[c_fov] = true;
                    /* if the cell is opaque, block the adjacent slopes */
                    if (walls_layer->data[c] != 0 || object.gid != 0) {
                        if (min_angle >= start_slope) {
                            min_angle = end_slope;
                            /* if min_angle is applied to the last cell in line, nothing more
                               needs to be checked. */
                            if (processed_cell == iteration) {
                                done = true;
                            }
                        } else if (!extended) {
                            start_angle[total_obstacles] = start_slope;
                            end_angle[total_obstacles++] = end_slope;
                        }
                        if (!light_walls) {
                            fov_array[c_fov] = false;
                        }
                    }
                }
                processed_cell++;
            }
            if (iteration == fov_max_distance) {
                done = true;
            }
            iteration++;
            obstacles_in_last_line = total_obstacles;
            y += dy;
            if (y < 0 || y >= static_cast<int32_t>(m.height)) {
                done = true;
            }
        }
    }

    /* octant: horizontal edge */
    {
        int32_t iteration = 1; /* iteration of the algo for this octant */
        bool done = false;
        int32_t total_obstacles = 0;
        int32_t obstacles_in_last_line = 0;
        double min_angle = 0.0;
        int32_t x;
        int32_t y;

        /* do while there are unblocked slopes left and the algo is within the map's boundaries
           scan progressive lines/columns from the PC outwards */
        x = player_x+dx; /*the outer slope's coordinates (first processed line) */
        if (x < 0 || x >= static_cast<int32_t>(m.width)) {
            done = true;
        }
        while (!done) {
            /* process cells in the line */
            double slopes_per_cell = 1.0 / (double)(iteration);
            double half_slopes = slopes_per_cell * 0.5;
            int32_t processed_cell = (int)((min_angle + half_slopes) / slopes_per_cell);
            int32_t miny = max(0, player_y - iteration);
            int32_t maxy = min(static_cast<int32_t>(m.height) - 1, player_y + iteration);
            done = true;
            for (y = player_y + (processed_cell * dy); y >= miny && y <= maxy; y += dy) {
                int32_t c = x + (y * m.width);
                int32_t c_fov = x - player_x + fov_max_distance + ((y - player_y + fov_max_distance) * fov_diameter);
                /* calculate slopes per cell */
                bool visible = true;
                bool extended = false;
                double centre_slope = (double)processed_cell * slopes_per_cell;
                double start_slope = centre_slope - half_slopes;
                double end_slope = centre_slope + half_slopes;
                auto const &object = opaque_layer->objects[c];

#ifdef LOG_FOV_EXTREME
                spdlog::trace("[{}] horizontal {}:{} c {} wall {}", __FUNCTION__, x, y, c, walls_layer->data[c]);
#endif

                if (obstacles_in_last_line > 0) {
                    if (!(fov_array[c_fov - dx] && walls_layer->data[c-dx] == 0 && opaque_layer->objects[c-dx].gid == 0) &&
                        !(fov_array[c_fov - (fov_diameter * dy) - dx] && walls_layer->data[c-(m.width * dy) - dx] == 0 && opaque_layer->objects[c-(m.width * dy) - dx].gid == 0))
                    {
                        visible = false;
                    } else {
                        for (int32_t idx = 0; idx < obstacles_in_last_line && visible; ++idx) {
                            if (start_slope <= end_angle[idx] && end_slope >= start_angle[idx]) {
                                if (walls_layer->data[c] != 0 || object.gid != 0) {
                                    if (centre_slope > start_angle[idx] && centre_slope < end_angle[idx]) {
                                        visible = false;
                                    }
                                } else {
                                    if (start_slope >= start_angle[idx] && end_slope <= end_angle[idx]) {
                                        visible = false;
                                    } else {
                                        start_angle[idx] = min(start_angle[idx], start_slope);
                                        end_angle[idx] = max(end_angle[idx], end_slope);
                                        extended = true;
                                    }
                                }
                                ++idx;
                            }
                        }
                    }
                }
                if (visible) {
                    done = false;
                    fov_array[c_fov] = true;
                    /* if the cell is opaque, block the adjacent slopes */
                    if (walls_layer->data[c] != 0 || object.gid != 0) {
                        if (min_angle >= start_slope) {
                            min_angle = end_slope;
                            /* if min_angle is applied to the last cell in line, nothing more
                               needs to be checked. */
                            if (processed_cell == iteration) {
                                done = true;
                            }
                        } else if (!extended) {
                            start_angle[total_obstacles] = start_slope;
                            end_angle[total_obstacles++] = end_slope;
                        }
                        if (!light_walls) {
                            fov_array[c_fov] = false;
                        }
                    }
                }
                processed_cell++;
            }
            if (iteration == fov_max_distance) {
                done = true;
            }
            iteration++;
            obstacles_in_last_line = total_obstacles;
            x += dx;
            if (x < 0 || x >= static_cast<int32_t>(m.width)) {
                done = true;
            }
        }
    }

    return fov_array;
}

bitset<power(fov_diameter)> lotr::compute_fov_restrictive_shadowcasting(map_component const &m, location &player_loc, bool const light_walls) {
    int max_obstacles;

    /* calculate an approximated (excessive, just in case) maximum number of obstacles per octant */
    max_obstacles = m.width * m.height / 7;

    /* check memory for angles */
    if (max_obstacles > allocated) {
        allocated = max_obstacles;
        start_angle = make_unique<double[]>(max_obstacles);
        end_angle = make_unique<double[]>(max_obstacles);
    }

    auto const &walls_layer = m.layers[map_layer_name::Walls];
    auto const &opaque_layer = m.layers[map_layer_name::OpaqueDecor];

    if(walls_layer.name.empty() || opaque_layer.name.empty()) {
        spdlog::error("[{}] missing walls or opaque layer for map {}", m.name);
        return {};
    }

    /* compute the 4 quadrants of the map */
    auto q1_fov = compute_fov_restrictive_shadowcasting_quadrant(m, &walls_layer, &opaque_layer, get<0>(player_loc), get<1>(player_loc), light_walls, 1, 1);
    auto q2_fov = compute_fov_restrictive_shadowcasting_quadrant(m, &walls_layer, &opaque_layer, get<0>(player_loc), get<1>(player_loc), light_walls, 1, -1);
    auto q3_fov = compute_fov_restrictive_shadowcasting_quadrant(m, &walls_layer, &opaque_layer, get<0>(player_loc), get<1>(player_loc), light_walls, -1, 1);
    auto q4_fov = compute_fov_restrictive_shadowcasting_quadrant(m, &walls_layer, &opaque_layer, get<0>(player_loc), get<1>(player_loc), light_walls, -1, -1);

#ifdef LOG_FOV_EXTREME
    log_fov(q1_fov | q2_fov | q3_fov | q4_fov, "fov");
#endif

    return q1_fov | q2_fov | q3_fov | q4_fov;
}

void lotr::log_fov(bitset<power(fov_diameter)> const &fov, string name) {
    for(int32_t y = fov_diameter - 1; y >= 0; y--) {
        string test = ""s;
        for(uint32_t x = 0; x < fov_diameter; x++) {
            test += fov[x + y * fov_diameter] == true ? "1"s : "0"s;
        }
        spdlog::trace("[{}] {} {}: {}", __FUNCTION__, name, y, test);
    }
}
