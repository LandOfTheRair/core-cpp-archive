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


#include "move_handler.h"

#include <spdlog/spdlog.h>

#include <messages/commands/move_request.h>
#include "message_handlers/handler_macros.h"

using namespace std;

namespace lotr {
    template <class WebSocket>
    void handle_move(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool, per_socket_data<WebSocket> *user_data,
            moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<WebSocket> *> user_connections) {
        DESERIALIZE_WITH_PLAYING_CHECK(move_request)

        q.enqueue(make_unique<player_move_message>(user_data->connection_id, msg->x, msg->y));
    }

    template void handle_move<uWS::WebSocket<true, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<uWS::WebSocket<true, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<true, true>> *> user_connections);
    template void handle_move<uWS::WebSocket<false, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<uWS::WebSocket<false, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<false, true>> *> user_connections);
}
