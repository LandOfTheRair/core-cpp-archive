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
#include <optional>
#include <rapidjson/document.h>
#include "../message.h"
#include "../item_object.h"
#include "../skill_object.h"

using namespace std;

namespace lotr {
    struct stat_component;
    struct character_allegiance {
        string name;
        string description;
        vector<stat_component> stat_mods;
        vector<item_object> items;
        vector<skill_object> skills;

        character_allegiance(string name, string description, vector<stat_component> stat_mods, vector<item_object> items, vector<skill_object> skills) noexcept;
    };

    struct character_class {
        string name;
        string description;
        vector<stat_component> stat_mods;

        character_class(string name, string description, vector<stat_component> stat_mods) noexcept;
    };

    struct character_select_response : public message {
        character_select_response(vector<stat_component> base_stats, vector<character_allegiance> allegiances, vector<character_class> classes) noexcept;

        ~character_select_response() noexcept = default;

        [[nodiscard]]
        string serialize() const override;

        [[nodiscard]]
        static optional<character_select_response> deserialize(rapidjson::Document const &d);

        vector<stat_component> base_stats;
        vector<character_allegiance> allegiances;
        vector<character_class> classes;

        static string const type;
    };
}
