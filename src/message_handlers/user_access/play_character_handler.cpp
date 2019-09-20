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
#include <repositories/characters_repository.h>
#include "message_handlers/handler_macros.h"
#include <ecs/components.h>

using namespace std;

namespace lotr {
    template <bool UseSsl>
    void handle_play_character(uWS::WebSocket<UseSsl, true> *ws, uWS::OpCode op_code, rapidjson::Document const &d,
                         shared_ptr<database_pool> pool, per_socket_data *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q) {
        DESERIALIZE_WITH_NOT_PLAYING_CHECK(play_character_request)

        characters_repository<database_pool, database_transaction> player_repo(pool);
        auto transaction = player_repo.create_transaction();
        auto plyr = player_repo.get_player(msg->name, included_tables::location, transaction);

        if(!plyr) {
            SEND_ERROR("Couldn't find character by name", "", "", true);
            return;
        }

        user_data->playing_character = true;

        vector<stat_component> player_stats;
        for(auto &stat : stat_names) {
            player_stats.emplace_back(stat, 10);
        }
        spdlog::debug("[{}] enqueing character {}", __FUNCTION__, plyr->name);
        q.enqueue(make_unique<player_enter_message>(plyr->name, plyr->loc->map_name, player_stats, user_data->connection_id, plyr->loc->x, plyr->loc->y));
    }

    template void handle_play_character<true>(uWS::WebSocket<true, true> *ws, uWS::OpCode op_code, rapidjson::Document const &d,
                                           shared_ptr<database_pool> pool, per_socket_data *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q);
    template void handle_play_character<false>(uWS::WebSocket<false, true> *ws, uWS::OpCode op_code, rapidjson::Document const &d,
                                            shared_ptr<database_pool> pool, per_socket_data *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q);
}
