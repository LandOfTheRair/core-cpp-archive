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


#include "public_chat_handler.h"

#include <spdlog/spdlog.h>

#include <messages/chat/message_request.h>
#include <messages/chat/message_response.h>
#include "message_handlers/handler_macros.h"
#include <game_logic/censor_sensor.h>

using namespace std;
using namespace chrono;

namespace lotr {
    template <class WebSocket>
    void handle_public_chat(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool, per_socket_data<WebSocket> *user_data,
            moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<WebSocket> *> user_connections) {
        DESERIALIZE_WITH_LOGIN_CHECK(message_request)

        auto now = system_clock::now();
        auto chat_msg = message_response(*user_data->username, sensor.clean_profanity_ish(msg->content), "game", duration_cast<milliseconds>(now.time_since_epoch()).count()).serialize();

        for (auto &[conn_id, other_user_data] : user_connections) {
            other_user_data->ws->send(chat_msg, op_code, true);
        }
    }

    template void handle_public_chat<uWS::WebSocket<true, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<uWS::WebSocket<true, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<true, true>> *> user_connections);
    template void handle_public_chat<uWS::WebSocket<false, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<uWS::WebSocket<false, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<false, true>> *> user_connections);
}
