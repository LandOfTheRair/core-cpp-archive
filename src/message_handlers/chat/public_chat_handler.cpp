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
#include <uws_thread.h>
#include "message_handlers/handler_macros.h"
#include <game_logic/censor_sensor.h>

using namespace std;

namespace lotr {
    template <bool UseSsl>
    void handle_public_chat(uWS::WebSocket<UseSsl, true> *ws, uWS::OpCode op_code, rapidjson::Document const &d,
                     shared_ptr<database_pool> pool, per_socket_data *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q) {
        DESERIALIZE_WITH_LOGIN_CHECK(message_request)

        auto chat_msg = message_response(*user_data->username, sensor.clean_profanity_ish(msg->content)).serialize();

        if constexpr (UseSsl) {
            for (auto &[conn_id, conn_ws] : user_ssl_connections) {
                conn_ws->send(chat_msg, op_code, true);
            }
        } else {
            for (auto &[conn_id, conn_ws] : user_connections) {
                conn_ws->send(chat_msg, op_code, true);
            }
        }
    }

    template void handle_public_chat<true>(uWS::WebSocket<true, true> *ws, uWS::OpCode op_code, rapidjson::Document const &d,
                                           shared_ptr<database_pool> pool, per_socket_data *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q);
    template void handle_public_chat<false>(uWS::WebSocket<false, true> *ws, uWS::OpCode op_code, rapidjson::Document const &d,
                                           shared_ptr<database_pool> pool, per_socket_data *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q);
}
