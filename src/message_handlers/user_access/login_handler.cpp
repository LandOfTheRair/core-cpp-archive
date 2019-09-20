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

#include <messages/user_access/login_request.h>
#include <messages/user_access/login_response.h>
#include <repositories/users_repository.h>
#include <repositories/banned_users_repository.h>
#include <repositories/characters_repository.h>
#include <on_leaving_scope.h>
#include "message_handlers/handler_macros.h"


using namespace std;
namespace lotr {
    template <bool UseSsl>
    void handle_login(uWS::WebSocket<UseSsl, true> *ws, uWS::OpCode op_code, rapidjson::Document const &d, shared_ptr<database_pool> pool,
                      per_socket_data *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q) {
        DESERIALIZE_WITH_NOT_LOGIN_CHECK(login_request)

        users_repository<database_pool, database_transaction> user_repo(pool);
        banned_users_repository<database_pool, database_transaction> banned_user_repo(pool);
        characters_repository<database_pool, database_transaction> player_repo(pool);

        auto transaction = user_repo.create_transaction();
        auto banned_usr = banned_user_repo.is_username_or_ip_banned(msg->username, {}, transaction);

        if (banned_usr) {
            ws->send("You are banned", op_code, true);
            ws->end(0);
            return;
        }

        auto usr = user_repo.get(msg->username, transaction);

        if (!usr) {
            SEND_ERROR("User already exists", "", "", true);
            return;
        }

        {
            sodium_mlock(reinterpret_cast<unsigned char *>(&msg->password[0]), msg->password.size());
            auto scope_guard = on_leaving_scope([&] {
                sodium_munlock(reinterpret_cast<unsigned char *>(&msg->password[0]), msg->password.size());
            });

            if (crypto_pwhash_str_verify(usr->password.c_str(), msg->password.c_str(), msg->password.length()) != 0) {
                SEND_ERROR("Password incorrect", "", "", true);
                return;
            }

            vector<message_player> message_players;
            auto players = player_repo.get_by_user_id(usr->id, included_tables::location, transaction);

            for (auto &player : players) {
                message_players.emplace_back(player.name, player.loc->map_name, player.loc->x, player.loc->y);
            }

            login_response response(message_players, usr->username, usr->email);
            auto response_msg = response.serialize();
            ws->send(response_msg, op_code, true);
            user_data->user_id = usr->id;
            user_data->username = new string(usr->username);
        }
    }

    template void handle_login<true>(uWS::WebSocket<true, true> *ws, uWS::OpCode op_code, rapidjson::Document const &d,
                                           shared_ptr<database_pool> pool, per_socket_data *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q);
    template void handle_login<false>(uWS::WebSocket<false, true> *ws, uWS::OpCode op_code, rapidjson::Document const &d,
                                            shared_ptr<database_pool> pool, per_socket_data *user_data, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &q);
}
