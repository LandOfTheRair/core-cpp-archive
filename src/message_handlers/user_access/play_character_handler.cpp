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
#include <repositories/stats_repository.h>
#include "message_handlers/handler_macros.h"
#include <ecs/components.h>
#include <uws_thread.h>
#include <utf.h>
#include <messages/user_access/character_select_response.h>

using namespace std;

namespace lotr {
    template <class Server, class WebSocket>
    void handle_play_character(Server *s, rapidjson::Document const &d,
                         shared_ptr<database_pool> pool, per_socket_data<WebSocket> *user_data, moodycamel::ConcurrentQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<WebSocket>> &user_connections) {
        MEASURE_TIME_OF_FUNCTION()
        DESERIALIZE_WITH_NOT_PLAYING_CHECK(play_character_request)

        characters_repository<database_pool, database_transaction> character_repo(pool);
        stats_repository<database_pool, database_transaction> stats_repo(pool);
        auto transaction = character_repo.create_transaction();
        auto character = character_repo.get_character_by_slot(msg->slot, user_data->user_id, included_tables::location, transaction);

        if(!character) {
            SEND_ERROR("Couldn't find character in that slot", "", "", true);
            return;
        }

        {
            shared_lock lock(user_connections_mutex);
            for (auto &[conn_id, other_user_data] : user_connections) {
                if (other_user_data.connection_id != user_data->connection_id && other_user_data.user_id == user_data->user_id &&
                    other_user_data.playing_character_slot == msg->slot) {
                    SEND_ERROR("Already playing that slot on another connection", "", "", true);
                    return;
                }
            }
        }

        user_data->playing_character_slot = msg->slot;
        auto db_stats = stats_repo.get_by_character_id(character->id, transaction);

        user_entered_game_response enter_msg(user_data->username);
        auto enter_msg_str = enter_msg.serialize();
        {
            shared_lock lock(user_connections_mutex);
            for (auto &[conn_id, other_user_data] : user_connections) {
                try {
                    if(other_user_data.ws.expired()) {
                        continue;
                    }

                    s->send(other_user_data.ws, enter_msg_str, websocketpp::frame::opcode::value::TEXT);
                } catch (...) {
                    continue;
                }
            }
        }

        // TODO move this calculation somewhere global
        auto allegiance_it = find_if(begin(select_response.allegiances), end(select_response.allegiances), [&allegiance = as_const(character->allegiance)](character_allegiance const &a){ return a.name == allegiance;});
        if(allegiance_it == end(select_response.allegiances)) {
            spdlog::error("[{}] character {} slot {} wrong allegiance", __FUNCTION__, character->name, character->slot, character->allegiance);
            SEND_ERROR("Chosen allegiance does not exist", "", "", true);
            return;
        }

        auto classes_it = find_if(begin(select_response.classes), end(select_response.classes), [&baseclass = as_const(character->_class)](character_class const &c){ return c.name == baseclass; });
        if(classes_it == end(select_response.classes)) {
            spdlog::error("[{}] character {} slot {} wrong class", __FUNCTION__, character->name, character->slot, character->_class);
            SEND_ERROR("Chosen class does not exist", "", "", true);
            return;
        }

        vector<stat_component> player_stats_mods;
        player_stats_mods.reserve(stat_names.size());
        for(auto const &stat : stat_names) {
            auto value = 0;
            auto allegiance_value_it = find_if(begin(allegiance_it->stat_mods), end(allegiance_it->stat_mods), [&stat](stat_component const &sc){ return sc.name == stat; });
            auto class_value_it = find_if(begin(classes_it->stat_mods), end(classes_it->stat_mods), [&stat](stat_component const &sc){ return sc.name == stat; });

            if(allegiance_value_it != end(allegiance_it->stat_mods)) {
                value += allegiance_value_it->value;
            }

            if(class_value_it != end(classes_it->stat_mods)) {
                value += class_value_it->value;
            }

            player_stats_mods.emplace_back(stat, value);
        }

        vector<stat_component> player_stats;
        player_stats.reserve(db_stats.size());
        for(auto const &stat : db_stats) {
            player_stats.emplace_back(stat.name, stat.value);
        }
        spdlog::debug("[{}] enqueing character {} slot {}", __FUNCTION__, character->name, character->slot);
        spdlog::trace("[{}] enqueing character {} has loc {}", __FUNCTION__, character->name, character->loc.has_value());
        q.enqueue(make_unique<player_enter_message>(character->name, character->gender, character->allegiance, character->_class, character->loc->map_name, move(player_stats),
                user_data->connection_id, character->level, character->gold, character->loc->x, character->loc->y));
    }

    template void handle_play_character<server, websocketpp::connection_hdl>(server *s, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<websocketpp::connection_hdl> *user_data, moodycamel::ConcurrentQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<websocketpp::connection_hdl>> &user_connections);
}
