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

namespace lotr {
    class custom_server {
    public:
        bool send(uint64_t handle, string message, websocketpp::frame::opcode::value op_code) {
            sent_message = message;

            return true;
        }

        void close(uint64_t handle, uint64_t code, string message) {
            close_message = message;
        }

        string sent_message;
        string close_message;
    };
}
