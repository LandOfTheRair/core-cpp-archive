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
#include <vector>

using namespace std;

namespace lotr {
    struct stat_component;

    struct queue_message {
        uint32_t type;

        queue_message(uint32_t type) : type(type) {}
        virtual ~queue_message() {}
    };

    struct player_enter_message : public queue_message {
        string character_name;
        string map_name;
        vector<stat_component> player_stats;
        uint64_t connection_id;
        uint32_t x;
        uint32_t y;

        player_enter_message(string character_name, string map_name, vector<stat_component> player_stats, uint64_t connection_id, uint32_t x, uint32_t y);
    };

    struct player_leave_message : public queue_message {
        string character_name;

        player_leave_message(string character_name);
    };
}
