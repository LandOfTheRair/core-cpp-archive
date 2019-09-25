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


#include "play_character_handler.h"

#include <spdlog/spdlog.h>

#include <messages/user_access/play_character_request.h>
#include <messages/user_access/user_entered_game_response.h>
#include <repositories/characters_repository.h>
#include "message_handlers/handler_macros.h"
#include <ecs/components.h>

using namespace std;

namespace lotr {
    template <class WebSocket>
    void handle_play_character(uWS::OpCode op_code, rapidjson::Document const &d,
                         shared_ptr<database_pool> pool, per_socket_data<WebSocket> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<WebSocket> *> user_connections) {
        DESERIALIZE_WITH_NOT_PLAYING_CHECK(play_character_request)

        characters_repository<database_pool, database_transaction> character_repo(pool);
        auto transaction = character_repo.create_transaction();
        auto character = character_repo.get_character_by_slot(msg->slot, user_data->user_id, included_tables::location, transaction);

        if(!character) {
            SEND_ERROR("Couldn't find character in that slot", "", "", true);
            return;
        }

        for (auto &[conn_id, other_user_data] : user_connections) {
            if(other_user_data->user_id == user_data->user_id && other_user_data->playing_character_slot == user_data->playing_character_slot) {
                SEND_ERROR("Already playing that slot on another connection", "", "", true);
                return;
            }
        }

        user_data->playing_character_slot = msg->slot;

        user_entered_game_response enter_msg(*user_data->username);
        auto enter_msg_str = enter_msg.serialize();
        for (auto &[conn_id, other_user_data] : user_connections) {
            other_user_data->ws->send(enter_msg_str, op_code, true);
        }

        vector<stat_component> player_stats;
        for(auto &stat : stat_names) {
            player_stats.emplace_back(stat, 10);
        }
        spdlog::debug("[{}] enqueing character {}", __FUNCTION__, character->name);
        spdlog::trace("[{}] enqueing character {} has loc {}", __FUNCTION__, character->name, character->loc.has_value());
        q.enqueue(make_unique<player_enter_message>(character->name, character->loc->map_name, player_stats, user_data->connection_id, character->loc->x, character->loc->y));
    }

    template void handle_play_character<uWS::WebSocket<true, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<uWS::WebSocket<true, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<true, true>> *> user_connections);
    template void handle_play_character<uWS::WebSocket<false, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<uWS::WebSocket<false, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<false, true>> *> user_connections);
}
