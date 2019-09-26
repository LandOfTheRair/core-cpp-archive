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

#include <string>
#include <rapidjson/writer.h>
#include "message.h"

using namespace std;

namespace lotr {
    struct stat_component;
    struct item_object {
        uint32_t tier;
        uint32_t value;
        uint32_t sprite;
        string name;
        string description;
        string type;
        vector<stat_component> stats;


        item_object(uint32_t tier, uint32_t value, uint32_t sprite, string name, string description, string type, vector<stat_component> stats) noexcept :
                tier(tier), value(value), sprite(sprite), name(move(name)), description(move(description)), type(move(type)), stats(move(stats)) {}
    };

    void write_item_object(rapidjson::Writer<rapidjson::StringBuffer> &writer, item_object const &obj);
}
