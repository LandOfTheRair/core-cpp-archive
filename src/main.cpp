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
#include <filesystem>
#include <rapidjson/document.h>
#include <message_handlers/login_handler.h>
#include <message_handlers/register_handler.h>

#include "config.h"
#include "logger_init.h"
#include "config_parsers.h"
#include "on_leaving_scope.h"

#include "messages/user_access/register_request.h"
#include "messages/user_access/login_response.h"

#include "repositories/users_repository.h"
#include "repositories/banned_users_repository.h"
#include "repositories/players_repository.h"
#include "working_directory_manipulation.h"
#include "per_socket_data.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"

using namespace std;
using namespace lotr;

atomic<uint64_t> connection_id_counter;

int main() {
    set_cwd(get_selfpath());

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

    if(!filesystem::exists("logs")) {
        if(!filesystem::create_directory("logs")) {
            spdlog::error("Fatal error creating logs directory");
            return -1;
        }
    }

    reconfigure_logger(config);

    auto pool = make_shared<database_pool>();
    pool->create_connections(config.connection_string, 1);

    users_repository<database_pool, database_transaction> user_repo(pool);
    banned_users_repository<database_pool, database_transaction> banned_user_repo(pool);
    players_repository<database_pool, database_transaction> player_repo(pool);

    uWS::TemplatedApp<false>()

    .ws<per_socket_data>("/*", {
        .compression = uWS::SHARED_COMPRESSOR,
        .maxPayloadLength = 16 * 1024,
        .idleTimeout = 10,
        .open = [](uWS::WebSocket<false, true> *ws, uWS::HttpRequest *req) {
            spdlog::trace("[uws] open connection {}", req->getUrl());
            //only called on connect
            auto *user_data = (per_socket_data*)ws->getUserData();
            user_data->connection_id = connection_id_counter++;
        },
        .message = [pool](auto *ws, std::string_view message, uWS::OpCode op_code) {
            spdlog::trace("[uws] message {} {}", message, op_code);

            if(message.empty() || message.length() < 4) {
                spdlog::warn("[uws] deserialize encountered empty buffer");
                return;
            }

            rapidjson::Document d;
            d.Parse(string(message).c_str()); // TODO see if we can prevent copying the string every time

            if (d.HasParseError() || !d.IsObject() || !d.HasMember("type") || !d["type"].IsString()) {
                spdlog::warn("[uws] deserialize failed");
                ws->end(0);
                return;
            }

            string type = d["type"].GetString();
            auto user_data = (per_socket_data*)ws->getUserData();

            if(type == "login") {
                handle_login(ws, op_code, d, pool, user_data);
            } else if (type == "register") {
                handle_register(ws, op_code, d, pool, user_data);
            }
        },
        .drain = [](auto *ws) {
            /* Check getBufferedAmount here */
            spdlog::trace("[uws] Something about draining {}", ws->getBufferedAmount());
        },
        .ping = [](auto *ws) {
            auto user_data = (per_socket_data*)ws->getUserData();
            spdlog::trace("[uws] ping from conn {} user {}", user_data->connection_id, user_data->user_id);
        },
        .pong = [](auto *ws) {
            auto user_data = (per_socket_data*)ws->getUserData();
            spdlog::trace("[uws] pong from conn {} user {}", user_data->connection_id, user_data->user_id);
        },
        .close = [](auto *ws, int code, std::string_view message) {
            //only called on close
            spdlog::trace("[uws] close connection {} {}", code, message);
        }
    })

    .listen(config.port, [&](auto *token) {
        if (token) {
            spdlog::info("[main] listening on \"{}:{}\"", config.address, config.port);
        }
    }).run();

    return 0;
}
#pragma clang diagnostic pop