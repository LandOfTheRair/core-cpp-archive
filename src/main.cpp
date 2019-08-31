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


#include <spdlog/spdlog.h>
#include <App.h>
#include <atomic>
#include <filesystem>
#include <rapidjson/document.h>
#include <functional>
#include <csignal>

#include <message_handlers/login_handler.h>
#include <message_handlers/register_handler.h>

#include <entt/entt.hpp>

#include "config.h"
#include "logger_init.h"
#include "config_parsers.h"
#include "map_loading/load_map.h"


#include "repositories/users_repository.h"
#include "repositories/banned_users_repository.h"
#include "repositories/players_repository.h"
#include "working_directory_manipulation.h"
#include "per_socket_data.h"
#include "lotr_flat_map.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"

using namespace std;
using namespace lotr;

atomic<uint64_t> connection_id_counter;
atomic<bool> quit{false};

void on_sigint(int sig) {
    quit = true;
}

struct uws_is_shit_struct {
    us_listen_socket_t *socket;
    uWS::Loop *loop;
};

int main() {
    set_cwd(get_selfpath());
    ::signal(SIGINT, on_sigint);

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

    lotr_flat_map<string, function<void(uWS::WebSocket<false, true> *, uWS::OpCode, rapidjson::Document const &, shared_ptr<database_pool>, per_socket_data*)>> message_router;
    message_router.emplace("Auth:login", handle_login);
    message_router.emplace("Auth:register", handle_register);
    uws_is_shit_struct shit_uws{}; // The documentation in uWS is appalling and the attitude the guy has is impossible to deal with. Had to search the issues of the github to find a method to close/stop uWS.

    auto uws_thread = thread([&config, pool, &message_router, &shit_uws] {
        shit_uws.loop = uWS::Loop::get();
        uWS::TemplatedApp<false>().
            ws<per_socket_data>("/*", {
                .compression = uWS::SHARED_COMPRESSOR,
                .maxPayloadLength = 16 * 1024,
                .idleTimeout = 10,
                .open = [](uWS::WebSocket<false, true> *ws, uWS::HttpRequest *req) {
                    if(quit) {
                        spdlog::debug("[uws] new connection in closing state");
                        ws->end(0);
                        return;
                    }

                    //only called on connect
                    auto *user_data = (per_socket_data *) ws->getUserData();
                    user_data->connection_id = connection_id_counter++;
                    user_data->user_id = 0;
                    spdlog::trace("[uws] open connection {} {}", req->getUrl(), user_data->connection_id);
                },
                .message = [pool, &message_router](auto *ws, std::string_view message, uWS::OpCode op_code) {
                    spdlog::trace("[uws] message {} {}", message, op_code);

                    if (message.empty() || message.length() < 4) {
                        spdlog::warn("[uws] deserialize encountered empty buffer");
                        return;
                    }

                    rapidjson::Document d;
                    d.Parse(&message[0], message.size());

                    if (d.HasParseError() || !d.IsObject() || !d.HasMember("type") || !d["type"].IsString()) {
                        spdlog::warn("[uws] deserialize failed");
                        ws->end(0);
                        return;
                    }

                    string type = d["type"].GetString();
                    auto user_data = (per_socket_data *) ws->getUserData();

                    auto handler = message_router.find(type);
                    if (handler != message_router.end()) {
                        handler->second(ws, op_code, d, pool, user_data);
                    } else {
                        spdlog::trace("[uws] no handler for type {}", type);
                    }
                },
                .drain = [](auto *ws) {
                    /* Check getBufferedAmount here */
                    spdlog::trace("[uws] Something about draining {}", ws->getBufferedAmount());
                },
                .ping = [](auto *ws) {
                    auto user_data = (per_socket_data *) ws->getUserData();
                    spdlog::trace("[uws] ping from conn {} user {}", user_data->connection_id, user_data->user_id);
                },
                .pong = [](auto *ws) {
                    auto user_data = (per_socket_data *) ws->getUserData();
                    spdlog::trace("[uws] pong from conn {} user {}", user_data->connection_id, user_data->user_id);
                },
                .close = [](auto *ws, int code, std::string_view message) {
                    //only called on close
                    auto *user_data = (per_socket_data *) ws->getUserData();
                    spdlog::trace("[uws] close connection {} {} {} {}", code, message, user_data->connection_id, user_data->user_id);
                }
        })

        .listen(config.port, [&](us_listen_socket_t *token) {
            if (token) {
                spdlog::info("[main] listening on \"{}:{}\"", config.address, config.port);
                shit_uws.socket = token;
            }
        }).run();

        spdlog::warn("[uws] done");
    });

    entt::registry registry;

    for(auto& p: filesystem::recursive_directory_iterator("assets/maps")) {
        if(!p.is_regular_file()) {
            continue;
        }

        auto map = load_map_from_file(p.path().string());

        if(!map) {
            continue;
        }

        auto new_entity = registry.create();
        registry.assign<map_component>(new_entity, map.value());
    }
    //world w;
    //w.load_from_database(db_pool, config, player_event_queue, producer);
    auto next_tick = chrono::system_clock::now() + chrono::milliseconds(config.tick_length);
    while (!quit) {
        auto now = chrono::system_clock::now();
        if(now < next_tick) {
            this_thread::sleep_until(next_tick);
        }
        //w.do_tick(config.tick_length);
        next_tick += chrono::milliseconds(config.tick_length);
    }

    spdlog::warn("quitting program");
    shit_uws.loop->defer([&shit_uws] {
        us_listen_socket_close(0, shit_uws.socket);
    });
    uws_thread.join();
    spdlog::warn("uws thread stopped");

    return 0;
}
#pragma clang diagnostic pop