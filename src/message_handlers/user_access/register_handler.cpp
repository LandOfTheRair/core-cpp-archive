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


#include "login_handler.h"

#include <spdlog/spdlog.h>
#include <sodium.h>

#include <messages/user_access/register_request.h>
#include <repositories/users_repository.h>
#include <repositories/banned_users_repository.h>
#include <repositories/characters_repository.h>
#include <repositories/stats_repository.h>
#include <on_leaving_scope.h>
#include <messages/user_access/login_response.h>
#include <game_logic/censor_sensor.h>
#include "message_handlers/handler_macros.h"
#include <utf.h>
#include <messages/user_access/user_joined_response.h>
#include <uws_thread.h>
#include <ecs/components.h>

#ifdef TEST_CODE
#include "../../../test/custom_server.h"
#endif

#define crypto_pwhash_argon2id_MEMLIMIT_rair 33554432U

using namespace std;


namespace lotr {
    template <class Server, class WebSocket>
    void handle_register(Server *s, rapidjson::Document const &d,
                         shared_ptr<database_pool> pool, per_socket_data<WebSocket> *user_data, moodycamel::ConcurrentQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<WebSocket>> &user_connections) {
        MEASURE_TIME_OF_FUNCTION()
        DESERIALIZE_WITH_NOT_LOGIN_CHECK(register_request)

        users_repository<database_pool, database_transaction> user_repo(pool);
        banned_users_repository<database_pool, database_transaction> banned_user_repo(pool);
        characters_repository<database_pool, database_transaction> player_repo(pool);
        stats_repository<database_pool, database_transaction> stats_repo(pool);

        if(sensor.is_profane_ish(msg->username)) {
            SEND_ERROR("Usernames cannot contain profanities", "", "", true);
            return;
        }

        if(To_UTF32(msg->username).size() < 2 || To_UTF32(msg->username).size() > 20) {
            SEND_ERROR("Usernames needs to be at least 2 characters and at most 20 characters", "", "", true);
            return;
        }

        if(To_UTF32(msg->password).size() < 8) {
            SEND_ERROR("Password needs to be at least 8 characters", "", "", true);
            return;
        }

        if(msg->password == msg->username) {
            SEND_ERROR("Password cannot equal username", "", "", true);
            return;
        }

        if(msg->password == msg->email) {
            SEND_ERROR("Password cannot equal email", "", "", true);
            return;
        }

        auto transaction = user_repo.create_transaction();
        // TODO modify uwebsockets to include ip address
        auto banned_usr = banned_user_repo.is_username_or_ip_banned(msg->username, {}, transaction);

        if (banned_usr) {
            s->close(user_data->ws, 0, "You are banned");
            return;
        }

        auto usr = user_repo.get(msg->username, transaction);

        if (usr) {
            SEND_ERROR("User already exists", "", "", true);
            return;
        }

        {
            sodium_mlock(reinterpret_cast<unsigned char *>(&msg->password[0]), msg->password.size());
            auto scope_guard = on_leaving_scope([&] {
                sodium_munlock(reinterpret_cast<unsigned char *>(&msg->password[0]), msg->password.size());
            });

            char hashed_password[crypto_pwhash_STRBYTES];

            if (crypto_pwhash_str(hashed_password,
                                  msg->password.c_str(),
                                  msg->password.length(),
                                  crypto_pwhash_argon2id_OPSLIMIT_SENSITIVE,
                                  crypto_pwhash_argon2id_MEMLIMIT_rair) != 0) {
                spdlog::error("Registering user, but out of memory?");
                s->send(user_data->ws, "server error", websocketpp::frame::opcode::value::TEXT);
                return;
            }

            user new_usr{0, msg->username, string(hashed_password), msg->email, 0, "", 0, 0};
            auto inserted = user_repo.insert_if_not_exists(new_usr, transaction);

            if (!inserted) {
                SEND_ERROR("Server error", "", "", true);
                return;
            }

            transaction->commit();

            user_data->user_id = new_usr.id;
            user_data->username = new_usr.username;

            vector<character_object> message_players;
            auto players = player_repo.get_by_user_id(usr->id, included_tables::all, transaction);

            for (auto &player : players) {
                auto db_stats = stats_repo.get_by_character_id(player.id, transaction);
                vector<stat_component> stats;
                stats.reserve(db_stats.size());
                for(auto const &stat : db_stats) {
                    stats.emplace_back(stat.name, stat.value);
                }
                vector<item_object> items;
                vector<skill_object> skills;
                message_players.emplace_back(player.name, player.gender, player.allegiance, player._class, player.loc->map_name, player.level, player.gold, player.loc->x, player.loc->y, move(stats), move(items), move(skills));
            }

            vector<account_object> online_users;
            user_joined_response join_msg(account_object(new_usr.is_game_master, false, false, 0, 0, new_usr.username));
            auto join_msg_str = join_msg.serialize();
            {
                shared_lock lock(user_connections_mutex);
                online_users.reserve(user_connections.size());
                for (auto &[conn_id, other_user_data] : user_connections) {
                    try {
                        if constexpr(is_same_v<WebSocket, websocketpp::connection_hdl>) {
                            if (other_user_data.ws.expired()) {
                                continue;
                            }
                        }
                        if (other_user_data.user_id != user_data->user_id) {
                            s->send(other_user_data.ws, join_msg_str, websocketpp::frame::opcode::value::TEXT);
                        }
                        if (!other_user_data.username.empty()) {
                            online_users.emplace_back(other_user_data.is_game_master, other_user_data.is_tester, false, 0, other_user_data.subscription_tier,
                                                      other_user_data.username);
                        }
                    } catch (...) {

                    }
                }
            }

            login_response response(move(message_players), move(online_users), new_usr.username, new_usr.email, motd);
            auto response_msg = response.serialize();
            s->send(user_data->ws, response_msg, websocketpp::frame::opcode::value::TEXT);
        }
    }

    template void handle_register<server, websocketpp::connection_hdl>(server *s, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<websocketpp::connection_hdl> *user_data, moodycamel::ConcurrentQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<websocketpp::connection_hdl>> &user_connections);

#ifdef TEST_CODE
    template void handle_register<custom_server, uint64_t>(custom_server *s, rapidjson::Document const &d, shared_ptr<database_pool> pool,
                                                           per_socket_data<uint64_t> *user_data, moodycamel::ConcurrentQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uint64_t>> &user_connections);
#endif
}
