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

using namespace std;

namespace lotr {
    struct stat_component {
        uint64_t database_id;
        string name;
        uint64_t value;
    };

    struct skill_component {
        uint64_t database_id;
        string name;
        uint64_t value;
    };

    struct item_component {
        uint64_t database_id;
        string name;
        vector<stat_component> stats;
    };

    struct location_component {
        string map_name;
        uint32_t x;
        uint32_t y;
    };

    struct silver_purchases_component {
        uint64_t database_id;
        string name;
        uint32_t count;
    };

    struct character_component {
        uint64_t database_id;
        string name;
        vector<stat_component> stats;
        vector<item_component> items;
        vector<skill_component> skills;
        vector<silver_purchases_component> silver_purchases;
        location_component location;
    };

    struct user_component {
        uint64_t database_id;
        string name;
        string email;
        uint16_t subscription_tier;
        bool is_tester;
        bool is_game_master;
        uint16_t trial_ends;
        bool has_done_trial;
        string discord_tag;
        bool discord_online;
        vector<character_component> characters;
    };
}