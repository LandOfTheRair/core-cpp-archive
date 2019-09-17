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

#include "uws_thread.h"
#include <spdlog/spdlog.h>
#include <App.h>
#include <lotr_flat_map.h>
#include <rapidjson/document.h>
#include <message_handlers/user_access/login_handler.h>
#include <message_handlers/user_access/register_handler.h>
#include <message_handlers/user_access/play_character_handler.h>
#include <message_handlers/user_access/create_character_handler.h>
#include <message_handlers/commands/move_handler.h>
#include <messages/user_access/login_request.h>
#include <messages/user_access/register_request.h>
#include <messages/user_access/play_character_request.h>
#include <messages/user_access/create_character_request.h>
#include <messages/commands/move_request.h>
#include "per_socket_data.h"

using namespace std;
using namespace lotr;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"

atomic<uint64_t> connection_id_counter = 0;

lotr_flat_map<uint64_t, uWS::WebSocket<false, true> *> lotr::user_connections;
moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> lotr::game_loop_queue;

void lotr::run_uws(config &config, shared_ptr<database_pool> pool, uws_is_shit_struct &shit_uws, atomic<bool> &quit) {
    connection_id_counter = 0;
    shit_uws.loop = uWS::Loop::get();

    lotr_flat_map<string, function<void(uWS::WebSocket<false, true> *, uWS::OpCode, rapidjson::Document const &, shared_ptr<database_pool>, per_socket_data*, moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &)>> message_router;
    message_router.emplace(login_request::type, handle_login);
    message_router.emplace(register_request::type, handle_register);
    message_router.emplace(play_character_request::type, handle_play_character);
    message_router.emplace(create_character_request::type, handle_create_character);
    message_router.emplace(move_request::type, handle_move);

    uWS::TemplatedApp<false>().
                    ws<per_socket_data>("/*", {
                    .compression = uWS::SHARED_COMPRESSOR,
                    .maxPayloadLength = 16 * 1024,
                    .idleTimeout = 300,
                    .open = [&](uWS::WebSocket<false, true> *ws, uWS::HttpRequest *req) {
                        if(quit) {
                            spdlog::debug("[uws] new connection in closing state");
                            ws->end(0);
                            return;
                        }

                        //only called on connect
                        auto *user_data = (per_socket_data *) ws->getUserData();
                        user_data->connection_id = connection_id_counter++;
                        user_data->user_id = 0;
                        user_data->playing_character = false;
                        user_connections[user_data->connection_id] = ws;
                        spdlog::debug("[uws] open connection {} {}", req->getUrl(), user_data->connection_id);
                    },
                    .message = [pool, &message_router](auto *ws, string_view message, uWS::OpCode op_code) {
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
                            handler->second(ws, op_code, d, pool, user_data, game_loop_queue);
                        } else {
                            spdlog::trace("[uws] no handler for type {}", type);
                        }
                    },
                    .drain = [](auto *ws) {
                        /* Check getBufferedAmount here */
                        spdlog::debug("[uws] Something about draining {}", ws->getBufferedAmount());
                    },
                    .ping = [](auto *ws) {
                        auto user_data = (per_socket_data *) ws->getUserData();
                        spdlog::debug("[uws] ping from conn {} user {}", user_data->connection_id, user_data->user_id);
                    },
                    .pong = [](auto *ws) {
                        auto user_data = (per_socket_data *) ws->getUserData();
                        spdlog::debug("[uws] pong from conn {} user {}", user_data->connection_id, user_data->user_id);
                    },
                    .close = [](auto *ws, int code, std::string_view message) {
                        //only called on close
                        auto *user_data = (per_socket_data *) ws->getUserData();
                        user_connections.erase(user_data->connection_id);
                        if(user_data->playing_character) {
                            game_loop_queue.enqueue(make_unique<player_leave_message>(user_data->connection_id));
                        }
                        spdlog::debug("[uws] close connection {} {} {} {}", code, message, user_data->connection_id, user_data->user_id);
                    }
            })

            .listen(config.port, [&](us_listen_socket_t *token) {
                if (token) {
                    spdlog::info("[uws] listening on \"{}:{}\"", config.address, config.port);
                    shit_uws.socket = token;
                }
            }).run();

    spdlog::warn("[uws] done");
}

#pragma clang diagnostic pop
