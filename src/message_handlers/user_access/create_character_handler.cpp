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
#include <messages/user_access/create_character_response.h>
#include <repositories/locations_repository.h>
#include <repositories/characters_repository.h>
#include <repositories/stats_repository.h>
#include <game_logic/censor_sensor.h>
#include "message_handlers/handler_macros.h"
#include <ecs/components.h>
#include <utf.h>
#include <uws_thread.h>
#include <messages/user_access/character_select_response.h>

using namespace std;

namespace lotr {
    template <class Server, class WebSocket>
    void handle_create_character(Server *s, rapidjson::Document const &d,
                               shared_ptr<database_pool> pool, per_socket_data<WebSocket> *user_data, moodycamel::ConcurrentQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<WebSocket>> &user_connections) {
        MEASURE_TIME_OF_FUNCTION()
        DESERIALIZE_WITH_NOT_PLAYING_CHECK(create_character_request)

        locations_repository<database_pool, database_transaction> location_repo(pool);
        characters_repository<database_pool, database_transaction> player_repo(pool);
        stats_repository<database_pool, database_transaction> stats_repo(pool);
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

        auto utf_name = To_UTF32(msg->name);
        if(utf_name.size() < 2 || utf_name.size() > 20) {
            SEND_ERROR("Character names needs to be at least 2 characters and at most 20 characters", "", "", true);
            return;
        }

        if(any_of(begin(msg->name), end(msg->name), ::isdigit)) {
            SEND_ERROR("Character names cannot contain digits", "", "", true);
            return;
        }

        if(any_of(begin(msg->name), end(msg->name), ::isspace)) {
            SEND_ERROR("Character names cannot contain spaces", "", "", true);
            return;
        }

        if(To_UTF32(utf_to_upper_copy(msg->name))[0] != To_UTF32(msg->name)[0]) {
            SEND_ERROR("Character names must start with a capital", "", "", true);
            return;
        }

        auto class_it = find_if(begin(select_response.classes), end(select_response.classes), [&baseclass = as_const(msg->baseclass)](character_class const &c){ return c.name == baseclass;});
        if(class_it == end(select_response.classes)) {
            SEND_ERROR("Selected class does not exist", "", "", true);
            spdlog::error("[{}] Attempted to create character with class {}", __FUNCTION__, msg->baseclass);
            return;
        }

        db_character new_player;
        new_player.name = msg->name;
        new_player.user_id = user_data->user_id;
        new_player.allegiance = msg->allegiance;
        new_player._class = msg->baseclass;
        new_player.gender = msg->gender;
        new_player.level = 1;
        db_location loc(0, "Tutorial", 14, 14);
        location_repo.insert(loc, transaction);
        new_player.loc = loc;
        new_player.location_id = loc.id;
        new_player.slot = msg->slot;

        auto allegiance_it = find_if(begin(select_response.allegiances), end(select_response.allegiances), [&allegiance = as_const(new_player.allegiance)](character_allegiance const &a){ return a.name == allegiance;});
        if(allegiance_it == end(select_response.allegiances)) {
            new_player.allegiance = "Undecided";
        }

        if(!player_repo.insert(new_player, transaction)) {
            SEND_ERROR("Player with name already exists", "", "", true);
            spdlog::error("[{}] Player with slot {} already exists, but this code path should never be hit.", __FUNCTION__, new_player.slot);
            return;
        }

        vector<stat_component> player_stats;
        player_stats.reserve(select_response.base_stats.size());
        for(auto const &stat : select_response.base_stats) {
            character_stat char_stat{0, new_player.id, stat.name, stat.value};
            stats_repo.insert(char_stat, transaction);
            player_stats.emplace_back(stat.name, stat.value);
        }

        transaction->commit();


        create_character_response response{character_object{new_player.name, new_player.gender, new_player.allegiance, new_player._class, new_player.loc->map_name,
                                                            new_player.level, new_player.gold, new_player.loc->x, new_player.loc->y, move(player_stats), {}, {}}};
        auto response_msg = response.serialize();
        s->send(user_data->ws, response_msg, websocketpp::frame::opcode::value::TEXT);
    }

    template void handle_create_character<server, websocketpp::connection_hdl>(server *s, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<websocketpp::connection_hdl> *user_data, moodycamel::ConcurrentQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<websocketpp::connection_hdl>> &user_connections);
}
