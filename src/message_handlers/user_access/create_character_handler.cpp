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

using namespace std;

namespace lotr {
    void handle_create_character(uWS::WebSocket<false, true> *ws, uWS::OpCode op_code, rapidjson::Document const &d,
                               shared_ptr<database_pool> pool, per_socket_data *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q) {
        DESERIALIZE_WITH_NOT_PLAYING_CHECK(create_character_request)

        locations_repository<database_pool, database_transaction> location_repo(pool);
        characters_repository<database_pool, database_transaction> player_repo(pool);
        auto transaction = player_repo.create_transaction();
        auto plyr = player_repo.get_player(msg->name, included_tables::location, transaction);

        if(plyr) {
            SEND_ERROR("Player with name already exists", "", "", true);
            return;
        }

        if(sensor.is_profane_ish(msg->name)) {
            SEND_ERROR("invalid_char_name", "Invalid Character Name", "That character name is not valid", true);
            return;
        }

        db_character new_player;
        new_player.name = msg->name;
        new_player.user_id = user_data->user_id;
        db_location loc(0, "Tutorial", 14, 14);
        location_repo.insert(loc, transaction);
        new_player.loc = loc;
        new_player.location_id = loc.id;

        if(!player_repo.insert(new_player, transaction)) {
            SEND_ERROR("Player with name already exists", "", "", true);
            spdlog::error("[{}] Player with name {} already exists, but this code path should never be hit.", __FUNCTION__, new_player.name);
            return;
        }

        transaction->commit();

        vector<stat_component> player_stats;
        for(auto &stat : stat_names) {
            player_stats.emplace_back(stat, 10);
        }
        create_character_response response{msg->name, player_stats};
        auto response_msg = response.serialize();
        if (!ws->send(response_msg, op_code, true)) {
            ws->end(0);
        }
    }
}
