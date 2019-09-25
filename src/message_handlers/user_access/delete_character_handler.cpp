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


#include "create_character_handler.h"

#include <spdlog/spdlog.h>

#include <messages/user_access/delete_character_request.h>
#include <repositories/characters_repository.h>
#include <messages/generic_ok_response.h>
#include "message_handlers/handler_macros.h"

using namespace std;

namespace lotr {
    template <class WebSocket>
    void handle_delete_character(uWS::OpCode op_code, rapidjson::Document const &d,
                                 shared_ptr<database_pool> pool, per_socket_data<WebSocket> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<WebSocket> *> user_connections) {
        DESERIALIZE_WITH_NOT_PLAYING_CHECK(delete_character_request)

        for (auto &[conn_id, other_user_data] : user_connections) {
            if(other_user_data->user_id == user_data->user_id && other_user_data->playing_character_slot >=0 && other_user_data->playing_character_slot == msg->slot) {
                SEND_ERROR("Already playing that slot on another connection", "", "", true);
                return;
            }
        }

        characters_repository<database_pool, database_transaction> player_repo(pool);
        auto transaction = player_repo.create_transaction();
        player_repo.delete_character_by_slot(msg->slot, user_data->user_id, transaction);
        transaction->commit();

        generic_ok_response response{fmt::format("Character in slot {} deleted", msg->slot)};
        auto response_msg = response.serialize();
        if (!user_data->ws->send(response_msg, op_code, true)) {
            user_data->ws->end(0);
        }
    }

    template void handle_delete_character<uWS::WebSocket<true, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
                                                                      per_socket_data<uWS::WebSocket<true, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<true, true>> *> user_connections);
    template void handle_delete_character<uWS::WebSocket<false, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
                                                                       per_socket_data<uWS::WebSocket<false, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<false, true>> *> user_connections);
}
