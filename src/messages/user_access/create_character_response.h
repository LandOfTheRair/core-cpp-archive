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
#include <vector>
#include <rapidjson/document.h>
#include "../message.h"

using namespace std;

namespace lotr {
    struct stat_component;
    struct create_character_response : public message {
        create_character_response(string name, vector<stat_component> stats) noexcept;

        ~create_character_response() noexcept = default;

        [[nodiscard]]
        string serialize() const override;

        [[nodiscard]]
        static optional<create_character_response> deserialize(rapidjson::Document const &d);

        string name;
        vector<stat_component> stats;

        static string const type;
    };
}
