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


#include "set_motd_handler.h"

#include <spdlog/spdlog.h>

#include <messages/moderator/set_motd_request.h>
#include <uws_thread.h>
#include <messages/moderator/update_motd_response.h>
#include "message_handlers/handler_macros.h"

using namespace std;

namespace lotr {
    template<class WebSocket>
    void set_motd_handler(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool, per_socket_data<WebSocket> *user_data,
                          moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<WebSocket> *> user_connections) {
        if(!user_data->is_game_master) {
            spdlog::warn("[{}] user {} tried to set motd but is not a game master!", __FUNCTION__, *user_data->username);
            return;
        }

        DESERIALIZE_WITH_PLAYING_CHECK(set_motd_request)

        spdlog::info("[{}] motd set to \"{}\" by user {}", __FUNCTION__, msg->motd, *user_data->username);
        motd = msg->motd;

        update_motd_response motd_msg(motd);
        auto motd_msg_str = motd_msg.serialize();
        for (auto &[conn_id, other_user_data] : user_connections) {
            other_user_data->ws->send(motd_msg_str, op_code, true);
        }
    }

    template void set_motd_handler<uWS::WebSocket<true, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
                                                               per_socket_data<uWS::WebSocket<true, true>> *user_data,
                                                               moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q,
                                                               lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<true, true>> *> user_connections);

    template void set_motd_handler<uWS::WebSocket<false, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
                                                                per_socket_data<uWS::WebSocket<false, true>> *user_data,
                                                                moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q,
                                                                lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<false, true>> *> user_connections);
}
