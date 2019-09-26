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
#include <on_leaving_scope.h>
#include <messages/user_access/login_response.h>
#include <game_logic/censor_sensor.h>
#include "message_handlers/handler_macros.h"
#include <utf.h>
#include <messages/user_access/user_joined_response.h>
#include <uws_thread.h>

#ifdef TEST_CODE
#include "../../../test/custom_websocket.h"
#endif

#define crypto_pwhash_argon2id_MEMLIMIT_rair 33554432U

using namespace std;


namespace lotr {
    template <class WebSocket>
    void handle_register(uWS::OpCode op_code, rapidjson::Document const &d,
                         shared_ptr<database_pool> pool, per_socket_data<WebSocket> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<WebSocket> *> user_connections) {
        MEASURE_TIME_OF_FUNCTION()
        DESERIALIZE_WITH_NOT_LOGIN_CHECK(register_request)

        users_repository<database_pool, database_transaction> user_repo(pool);
        banned_users_repository<database_pool, database_transaction> banned_user_repo(pool);
        characters_repository<database_pool, database_transaction> player_repo(pool);

        if(sensor.is_profane_ish(msg->username)) {
            SEND_ERROR("Usernames cannot contain profanities", "", "", true);
            return;
        }

        if(To_UTF16(msg->username).size() < 2 || To_UTF16(msg->username).size() > 20) {
            SEND_ERROR("Usernames needs to be at least 2 characters and at most 20 characters", "", "", true);
            return;
        }

        if(To_UTF16(msg->password).size() < 8) {
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
            user_data->ws->send("You are banned", op_code, true);
            user_data->ws->end(0);
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
                if (!user_data->ws->send("Server error", op_code, true)) {
                    user_data->ws->end(0);
                }
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
            user_data->username = new string(new_usr.username);


            vector<account_object> online_users;
            user_joined_response join_msg(account_object(new_usr.is_game_master, false, false, 0, 0, new_usr.username));
            auto join_msg_str = join_msg.serialize();
            for (auto &[conn_id, other_user_data] : user_connections) {
                if(other_user_data->user_id != user_data->user_id) {
                    other_user_data->ws->send(join_msg_str, op_code, true);
                }
                if(other_user_data->username != nullptr) {
                    online_users.emplace_back(other_user_data->is_game_master, other_user_data->is_tester, false, 0, other_user_data->subscription_tier, *other_user_data->username);
                }
            }

            login_response response({}, online_users, new_usr.username, new_usr.email, motd);
            auto response_msg = response.serialize();
            if (!user_data->ws->send(response_msg, op_code, true)) {
                user_data->ws->end(0);
            }
        }
    }

    template void handle_register<uWS::WebSocket<true, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<uWS::WebSocket<true, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<true, true>> *> user_connections);
    template void handle_register<uWS::WebSocket<false, true>>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<uWS::WebSocket<false, true>> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<false, true>> *> user_connections);

#ifdef TEST_CODE
    template void handle_register<custom_websocket>(uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
            per_socket_data<custom_websocket> *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q, lotr_flat_map<uint64_t, per_socket_data<custom_websocket> *> user_connections);
#endif
}
