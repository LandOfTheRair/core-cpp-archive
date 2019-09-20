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
    struct message_player {
        string name;
        string map_name;
        uint32_t x;
        uint32_t y;

        message_player(string name, string map_name, uint32_t x, uint32_t y)
            : name(move(name)), map_name(move(map_name)), x(x), y(y) {}
    };

    struct login_response : public message {
        explicit login_response(vector<message_player> players, string username, string email) noexcept;

        ~login_response() noexcept = default;

        [[nodiscard]]
        string serialize() const override;
        static optional<login_response> deserialize(rapidjson::Document const &d);

        vector<message_player> players;
        string username;
        string email;

        static string const type;
    };
}
