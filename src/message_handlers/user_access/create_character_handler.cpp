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

#include <messages/user_access/create_character_request.h>
#include <repositories/locations_repository.h>
#include <repositories/characters_repository.h>
#include <game_logic/censor_sensor.h>
#include <messages/user_access/create_character_response.h>
#include "message_handlers/handler_macros.h"
#include <ecs/components.h>
#include <utf.h>

using namespace std;

namespace lotr {
    template <class WebSocket>
    void handle_create_character(uWS::OpCode op_code, rapidjson::Document const &d,
                               shared_ptr<database_pool> pool, per_socket_data<WebSocket> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<WebSocket> *> user_connections) {
        DESERIALIZE_WITH_NOT_PLAYING_CHECK(create_character_request)

        locations_repository<database_pool, database_transaction> location_repo(pool);
        characters_repository<database_pool, database_transaction> player_repo(pool);
        auto transaction = player_repo.create_transaction();
        auto existing_character = player_repo.get_character_by_slot(msg->slot, user_data->user_id, included_tables::location, transaction);

        if(existing_character) {
            SEND_ERROR("Character already exists in slot", "", "", true);
            return;
        }

        if(sensor.is_profane_ish(msg->name)) {
            SEND_ERROR("invalid_char_name", "Invalid Character Name", "That character name is not valid", true);
            return;
        }

        if(To_UTF16(msg->name).size() < 2 || To_UTF16(msg->name).size() > 20) {
            SEND_ERROR("Character names needs to be at least 2 characters and at most 20 characters", "", "", true);
            return;
        }

        db_character new_player;
        new_player.name = msg->name;
        new_player.user_id = user_data->user_id;
        db_location loc(0, "Tutorial", 14, 14);
        location_repo.insert(loc, transaction);
        new_player.loc = loc;
        new_player.location_id = loc.id;
        new_player.slot = msg->slot;

        if(!player_repo.insert(new_player, transaction)) {
            SEND_ERROR("Player with name already exists", "", "", true);
            spdlog::error("[{}] Player with slot {} already exists, but this code path should never be hit.", __FUNCTION__, new_player.slot);
            return;
        }

        transaction->commit();

        vector<stat_component> player_stats;
        for(auto &stat : stat_names) {
            player_stats.emplace_back(stat, 10);
        }
        create_character_response response{msg->name, player_stats};
        auto response_msg = response.serialize();
        if (!user_data->ws->send(response_msg, op_code, true)) {
            user_data->ws->end(0);
        }
    }

    template void handle_create_character<uWS::WebSocket<true, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<uWS::WebSocket<true, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<true, true>> *> user_connections);
    template void handle_create_character<uWS::WebSocket<false, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<uWS::WebSocket<false, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<false, true>> *> user_connections);
}
