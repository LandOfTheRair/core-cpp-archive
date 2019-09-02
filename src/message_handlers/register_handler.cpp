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
#include <repositories/players_repository.h>
#include <on_leaving_scope.h>
#include <messages/user_access/login_response.h>
#include <censor_sensor.h>


using namespace std;
namespace lotr {
    void handle_register(uWS::WebSocket<false, true> *ws, uWS::OpCode op_code, rapidjson::Document const &d,
                         shared_ptr<database_pool> pool, per_socket_data *user_data) {
        auto msg = register_request::deserialize(d);
        static censor_sensor s("assets/profanity_locales/en.json");

        if (!msg) {
            ws->send("Stop messing around", op_code, true);
            ws->end(0);
            return;
        }

        users_repository<database_pool, database_transaction> user_repo(pool);
        banned_users_repository<database_pool, database_transaction> banned_user_repo(pool);
        players_repository<database_pool, database_transaction> player_repo(pool);

        if(s.is_profane_ish(msg->username)) {
            ws->send("Usernames cannot contain profanities", op_code, true);
            return;
        }

        auto transaction = user_repo.create_transaction();
        // TODO modify uwebsockets to include ip address
        auto banned_usr = banned_user_repo.is_username_or_ip_banned(msg->username, {}, transaction);

        if (banned_usr) {
            ws->send("You are banned", op_code, true);
            ws->end(0);
            return;
        }

        auto usr = user_repo.get(msg->username, transaction);

        if (usr) {
            if (!ws->send("User already exists", op_code, true)) {
                ws->end(0);
            } // TODO figure out how to not have to do this shit every time I want to send stuff
            return;
        }

        {
            sodium_mlock(reinterpret_cast<unsigned char *>(&msg->password[0]), msg->password.size());
            auto scope_guard = on_leaving_scope([&] {
                sodium_munlock(reinterpret_cast<unsigned char *>(&msg->password[0]), msg->password.size());
            });

            char hashed_password[crypto_pwhash_STRBYTES];

            // TODO benchmark this, having increased the no. of passes to the recommended 10 or higher,
            //  this might result in taking a huge amount of time
            if (crypto_pwhash_str(hashed_password,
                                  msg->password.c_str(),
                                  msg->password.length(),
                                  10,
                                  crypto_pwhash_argon2id_MEMLIMIT_INTERACTIVE) != 0) {
                spdlog::error("Registering user, but out of memory?");
                if (!ws->send("Server error", op_code, true)) {
                    ws->end(0);
                }
                return;
            }

            user new_usr{0, msg->username, string(hashed_password), msg->email, 0, "", 0, 0};
            auto inserted = user_repo.insert_if_not_exists(new_usr, transaction);

            if (!inserted) {
                if (!ws->send("Server error", op_code, true)) {
                    ws->end(0);
                }
                return;
            }

            login_response response({});
            auto response_msg = response.serialize();
            if (!ws->send(response_msg, op_code, true)) {
                ws->end(0);
            }
            user_data->user_id = new_usr.id;
        }
    }
}