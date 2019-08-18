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


#include <iostream>
#include <spdlog/spdlog.h>
#include <optional>
#include <App.h>
#include <atomic>
#include <sodium.h>

#include "config.h"
#include "logger_init.h"
#include "config_parsers.h"
#include "on_leaving_scope.h"

#include "messages/user_access/register_request.h"
#include "messages/user_access/login_response.h"

#include "repositories/users_repository.h"
#include "repositories/banned_users_repository.h"
#include "repositories/players_repository.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
using namespace std;
using namespace lotr;

struct PerSocketData {
    uint32_t connection_id;
    uint32_t user_id;

    PerSocketData() : connection_id(0), user_id(0) {}
};

atomic<uint64_t> connection_id_counter;

int main() {
    config config;
    try {
        auto config_opt = parse_env_file();
        if(!config_opt) {
            return 1;
        }
        config = config_opt.value();
    } catch (const exception& e) {
        spdlog::error("[main] config.json file is malformed json.");
        return 1;
    }

    reconfigure_logger(config);

    auto pool = make_shared<database_pool>();
    pool->create_connections(config.connection_string, 1);

    users_repository<database_pool, database_transaction> user_repo(pool);
    banned_users_repository<database_pool, database_transaction> banned_user_repo(pool);
    players_repository<database_pool, database_transaction> player_repo(pool);

    uWS::TemplatedApp<false, PerSocketData>()

    .ws("/login", {
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024,
        .idleTimeout = 10,
        .open = [](auto *ws, auto *req) {},
        .message = [&](auto *ws, std::string_view message, uWS::OpCode opCode) {
            auto msg = register_request::deserialize(string(message));

            if(!msg) {
                ws->send("Stop messing around", opCode, true);
                ws->end(0);
                return;
            }

            auto transaction = user_repo.create_transaction();
            auto banned_usr = banned_user_repo.is_username_or_ip_banned(msg->username, {}, transaction);

            if(banned_usr) {
                ws->send("You are banned", opCode, true);
                ws->end(0);
                return;
            }

            auto usr = user_repo.get(msg->username, transaction);

            if(!usr) {
                if(!ws->send("User does not exist", opCode, true)) {
                    ws->end(0);
                }
                return;
            }

            {
                sodium_mlock(reinterpret_cast<unsigned char *>(msg->password[0]), msg->password.size());
                auto scope_guard = on_leaving_scope([&] {
                    sodium_munlock(reinterpret_cast<unsigned char *>(msg->password[0]), msg->password.size());
                });

                if(crypto_pwhash_str_verify(usr->password.c_str(), msg->password.c_str(), msg->password.length()) != 0) {
                    if(!ws->send("Password incorrect", opCode, true)) {
                        ws->end(0);
                    }
                    return;
                }

                vector<message_player> message_players;
                auto players = player_repo.get_by_user_id(usr->id, included_tables::location, transaction);

                for(auto &player : players) {
                    message_players.emplace_back(player.name, player.loc->map_name, player.loc->x, player.loc->y);
                }

                login_response response(message_players);
                auto response_msg = response.serialize();
                ws->send(response_msg);

                PerSocketData *user_data = ws->getUserData();
                user_data->user_id = usr->id;
            }
        },
        .drain = [](auto *ws) {
            /* Check getBufferedAmount here */
            spdlog::trace("Something about draining {}", ws->getBufferedAmount());
        },
        .ping = [](auto *ws) {},
        .pong = [](auto *ws) {},
        .close = [](auto *ws, int code, std::string_view message) {}
    })

    .ws("/register", {
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024,
        .idleTimeout = 10,
        .open = [](auto *ws, auto *req) {},
        .message = [&](auto *ws, std::string_view message, uWS::OpCode opCode) {
            auto msg = register_request::deserialize(string(message)); // TODO figure out how to not make a copy every time

            if(!msg) {
                ws->send("Stop messing around", opCode, true);
                ws->end(0);
                return;
            }

            auto transaction = user_repo.create_transaction();
            // TODO modify uwebsockets to include ip address
            auto banned_usr = banned_user_repo.is_username_or_ip_banned(msg->username, {}, transaction);

            if(banned_usr) {
                ws->send("You are banned", opCode, true);
                ws->end(0);
                return;
            }

            auto usr = user_repo.get(msg->username, transaction);

            if(usr) {
                if(!ws->send("User already exists", opCode, true)) {
                    ws->end(0);
                } // TODO figure out how to not have to do this shit every time I want to send stuff
                return;
            }

            {
                sodium_mlock(reinterpret_cast<unsigned char *>(msg->password[0]), msg->password.size());
                auto scope_guard = on_leaving_scope([&] {
                    sodium_munlock(reinterpret_cast<unsigned char *>(msg->password[0]), msg->password.size());
                });

                char hashed_password[crypto_pwhash_STRBYTES];

                // TODO benchmark this, having increased the no. of passes to the recommended 10 or higher,
                //  this might result in taking a huge amount of time
                if(crypto_pwhash_str(hashed_password,
                                     msg->password.c_str(),
                                     msg->password.length(),
                                     10,
                                     crypto_pwhash_argon2id_MEMLIMIT_INTERACTIVE) != 0) {
                    spdlog::error("Registering user, but out of memory?");
                    if(!ws->send("Server error", opCode, true)) {
                        ws->end(0);
                    }
                    return;
                }

                user new_usr{0, msg->username, string(hashed_password), msg->email, 0, "", 0, 0};
                auto inserted = user_repo.insert_if_not_exists(new_usr, transaction);

                if(!inserted) {
                    if(!ws->send("Server error", opCode, true)) {
                        ws->end(0);
                    }
                    return;
                }

                login_response response({});
                auto response_msg = response.serialize();
                if(!ws->send(response_msg, opCode, true)) {
                    ws->end(0);
                }
                PerSocketData *user_data = ws->getUserData();
                user_data->user_id = new_usr.id;
            }
        },
        .drain = [](auto *ws) {
            /* Check getBufferedAmount here */
            spdlog::trace("Something about draining {}", ws->getBufferedAmount());
        },
        .ping = [](auto *ws) {},
        .pong = [](auto *ws) {},
        .close = [](auto *ws, int code, std::string_view message) {}
    })

    .ws("/*", {
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024,
        .idleTimeout = 10,
        .open = [](auto *ws, auto *req) {
            //only called on connect
            PerSocketData *user_data = ws->getUserData();
            user_data->connection_id = connection_id_counter++;
        },
        .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
            ws->send(message, opCode, true);
        },
        .drain = [](auto *ws) {
            /* Check getBufferedAmount here */
            spdlog::trace("Something about draining {}", ws->getBufferedAmount());
        },
        .ping = [](auto *ws) {

        },
        .pong = [](auto *ws) {

        },
        .close = [](auto *ws, int code, std::string_view message) {
            //only called on close
        }
    })

    .post("/register", [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {

        //auto msg = register_request::deserialize(req->getParameter())

        res->writeStatus(uWS::HTTP_200_OK);
        res->end("Hello world!");
    })

    .listen(3000, [&](auto *token) {
        if (token) {
            spdlog::info("[main] listening on \"{}:{}\"", config.address, config.port);
        }
    }).run();

    return 0;
}
#pragma clang diagnostic pop