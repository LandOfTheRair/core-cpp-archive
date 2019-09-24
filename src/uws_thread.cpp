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
#include <message_handlers/user_access/delete_character_handler.h>
#include <message_handlers/commands/move_handler.h>
#include <message_handlers/chat/public_chat_handler.h>
#include <message_handlers/moderator/set_motd_handler.h>
#include <messages/user_access/login_request.h>
#include <messages/user_access/register_request.h>
#include <messages/user_access/play_character_request.h>
#include <messages/user_access/create_character_request.h>
#include <messages/user_access/delete_character_request.h>
#include <messages/commands/move_request.h>
#include <messages/chat/message_request.h>
#include <messages/moderator/set_motd_request.h>
#include <message_handlers/handler_macros.h>
#include <messages/user_access/user_left_response.h>
#include "per_socket_data.h"

using namespace std;
using namespace lotr;

template <bool UseSsl>
using message_router_type = lotr_flat_map<string, function<void(uWS::OpCode, rapidjson::Document const &, shared_ptr<database_pool>, per_socket_data<uWS::WebSocket<UseSsl, true>>*,
        moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> &, lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<UseSsl, true>> *> user_connections)>>;

atomic<uint64_t> connection_id_counter = 0;
uint32_t const ws_timeout = 300;
uint32_t const ws_max_payload = 16 * 1024;


lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<false, true>> *> lotr::user_connections;
lotr_flat_map<uint64_t, per_socket_data<uWS::WebSocket<true, true>> *> lotr::user_ssl_connections;
moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> lotr::game_loop_queue;
string lotr::motd;

template <bool UseSsl>
void on_open(atomic<bool> &quit, uWS::WebSocket<UseSsl, true> *ws, uWS::HttpRequest *req) {
    if(quit) {
        spdlog::debug("[{}] new connection in closing state", __FUNCTION__);
        ws->end(0);
        return;
    }

    //only called on connect
    auto *user_data = (per_socket_data<uWS::WebSocket<UseSsl, true>> *) ws->getUserData();
    user_data->connection_id = connection_id_counter++;
    user_data->user_id = 0;
    user_data->playing_character = false;
    user_data->username = nullptr;
    user_data->ws = ws;
    user_data->subscription_tier = 0;
    if constexpr(UseSsl) {
        user_ssl_connections[user_data->connection_id] = user_data;
    } else {
        user_connections[user_data->connection_id] = user_data;
    }
    spdlog::debug("[{}] open connection {} {}", __FUNCTION__, req->getUrl(), user_data->connection_id);
}

template <bool UseSsl>
void on_message(shared_ptr<database_pool> pool, message_router_type<UseSsl> &message_router, uWS::WebSocket<UseSsl, true> *ws, string_view message, uWS::OpCode op_code) {
    spdlog::trace("[{}] message {} {}", __FUNCTION__, message, op_code);

    if (message.empty() || message.length() < 4) {
        spdlog::warn("[{}] deserialize encountered empty buffer", __FUNCTION__);
        return;
    }

    auto user_data = (per_socket_data<uWS::WebSocket<UseSsl, true>> *) ws->getUserData();

    rapidjson::Document d;
    d.Parse(&message[0], message.size());

    if (d.HasParseError() || !d.IsObject() || !d.HasMember("type") || !d["type"].IsString()) {
        spdlog::warn("[{}] deserialize failed", __FUNCTION__);
        SEND_ERROR("Unrecognized message", "", "", true);
        return;
    }

    string type = d["type"].GetString();

    auto handler = message_router.find(type);
    if (handler != message_router.end()) {
        if constexpr (UseSsl){
            handler->second(op_code, d, pool, user_data, game_loop_queue, user_ssl_connections);
        } else {
            handler->second(op_code, d, pool, user_data, game_loop_queue, user_connections);
        }
    } else {
        spdlog::trace("[{}] no handler for type {}", __FUNCTION__, type);
    }
}

template <bool UseSsl>
void on_close(uWS::WebSocket<UseSsl, true> *ws, int code, std::string_view message) {
    auto *user_data = (per_socket_data<uWS::WebSocket<UseSsl, true>> *) ws->getUserData();
    if constexpr(UseSsl) {
        user_ssl_connections.erase(user_data->connection_id);
    } else {
        user_connections.erase(user_data->connection_id);
    }
    if(user_data->playing_character) {
        game_loop_queue.enqueue(make_unique<player_leave_message>(user_data->connection_id));
    }
    if(user_data->username != nullptr) {
        if constexpr(UseSsl) {
            user_left_response join_msg(*user_data->username);
            auto join_msg_str = join_msg.serialize();
            for (auto &[conn_id, other_user_data] : user_ssl_connections) {
                if(other_user_data->user_id != user_data->user_id) {
                    other_user_data->ws->send(join_msg_str, uWS::OpCode::TEXT, true);
                }
            }

            user_ssl_connections.erase(user_data->connection_id);
        } else {
            user_left_response join_msg(*user_data->username);
            auto join_msg_str = join_msg.serialize();
            for (auto &[conn_id, other_user_data] : user_connections) {
                if(other_user_data->user_id != user_data->user_id) {
                    other_user_data->ws->send(join_msg_str, uWS::OpCode::TEXT, true);
                }
            }

            user_connections.erase(user_data->connection_id);
        }
        delete user_data->username;
    }
    spdlog::debug("[{}] close connection {} {} {} {}", __FUNCTION__, code, message, user_data->connection_id, user_data->user_id);
}

template <bool UseSsl>
void add_routes(message_router_type<UseSsl> &message_router) {
    message_router.emplace(login_request::type, handle_login<uWS::WebSocket<UseSsl, true>>);
    message_router.emplace(register_request::type, handle_register<uWS::WebSocket<UseSsl, true>>);
    message_router.emplace(play_character_request::type, handle_play_character<uWS::WebSocket<UseSsl, true>>);
    message_router.emplace(create_character_request::type, handle_create_character<uWS::WebSocket<UseSsl, true>>);
    message_router.emplace(delete_character_request::type, handle_delete_character<uWS::WebSocket<UseSsl, true>>);
    message_router.emplace(move_request::type, handle_move<uWS::WebSocket<UseSsl, true>>);
    message_router.emplace(message_request::type, handle_public_chat<uWS::WebSocket<UseSsl, true>>);
    message_router.emplace(set_motd_request::type, set_motd_handler<uWS::WebSocket<UseSsl, true>>);
}

void lotr::run_uws(config &config, shared_ptr<database_pool> pool, uws_is_shit_struct &shit_uws, atomic<bool> &quit) {
    connection_id_counter = 0;
    shit_uws.loop = uWS::Loop::get();
    motd = "";

    if(config.use_ssl) {
        message_router_type<true> message_router;
        add_routes(message_router);

        uWS::TemplatedApp<true>({
                                        /*const char *key_file_name;
                                        const char *cert_file_name;
                                        const char *passphrase;
                                        const char *dh_params_file_name;
                                        int ssl_prefer_low_memory_usage;*/
                        .key_file_name = "key.pem",
                        .cert_file_name = "cert.pem",
                        .passphrase = nullptr,
                        .dh_params_file_name = nullptr,
                        .ssl_prefer_low_memory_usage = false
                        }).
                        ws<per_socket_data<uWS::WebSocket<true, true>>>("/*", {
                        .compression = uWS::SHARED_COMPRESSOR,
                        .maxPayloadLength = ws_max_payload,
                        .idleTimeout = ws_timeout,
                        .open = [&](auto *ws, uWS::HttpRequest *req) {
                            on_open(quit, ws, req);
                        },
                        .message = [pool, &message_router](auto *ws, string_view message, uWS::OpCode op_code) {
                            on_message(pool, message_router, ws, message, op_code);
                        },
                        .drain = [](auto *ws) {
                            /* Check getBufferedAmount here */
                            spdlog::debug("[uws] Something about draining {}", ws->getBufferedAmount());
                        },
                        .ping = [](auto *ws) {
                            auto user_data = (per_socket_data<uWS::WebSocket<true, true>> *) ws->getUserData();
                            spdlog::debug("[uws] ping from conn {} user {}", user_data->connection_id, user_data->user_id);
                        },
                        .pong = [](auto *ws) {
                            auto user_data = (per_socket_data<uWS::WebSocket<true, true>> *) ws->getUserData();
                            spdlog::debug("[uws] pong from conn {} user {}", user_data->connection_id, user_data->user_id);
                        },
                        .close = [](auto *ws, int code, std::string_view message) {
                            on_close(ws, code, message);
                        }
                })

                .listen(config.port, [&](us_listen_socket_t *token) {
                    if (token) {
                        spdlog::info("[uws] listening_ssl on \"{}:{}\"", config.address, config.port);
                        shit_uws.socket = token;
                    }
                }).run();
    } else {
        message_router_type<false> message_router;
        add_routes(message_router);

        uWS::TemplatedApp<false>().
                        ws<per_socket_data<uWS::WebSocket<false, true>>>("/*", {
                        .compression = uWS::SHARED_COMPRESSOR,
                        .maxPayloadLength = ws_max_payload,
                        .idleTimeout = ws_timeout,
                        .open = [&](auto *ws, uWS::HttpRequest *req) {
                            on_open(quit, ws, req);
                        },
                        .message = [pool, &message_router](auto *ws, string_view message, uWS::OpCode op_code) {
                            on_message(pool, message_router, ws, message, op_code);
                        },
                        .drain = [](auto *ws) {
                            /* Check getBufferedAmount here */
                            spdlog::debug("[uws] Something about draining {}", ws->getBufferedAmount());
                        },
                        .ping = [](auto *ws) {
                            auto user_data = (per_socket_data<uWS::WebSocket<false, true>> *) ws->getUserData();
                            spdlog::debug("[uws] ping from conn {} user {}", user_data->connection_id, user_data->user_id);
                        },
                        .pong = [](auto *ws) {
                            auto user_data = (per_socket_data<uWS::WebSocket<false, true>> *) ws->getUserData();
                            spdlog::debug("[uws] pong from conn {} user {}", user_data->connection_id, user_data->user_id);
                        },
                        .close = [](auto *ws, int code, std::string_view message) {
                            on_close(ws, code, message);
                        }
                })

                .listen(config.port, [&](us_listen_socket_t *token) {
                    if (token) {
                        spdlog::info("[uws] listening on \"{}:{}\"", config.address, config.port);
                        shit_uws.socket = token;
                    }
                }).run();
    }

    spdlog::warn("[{}] done", __FUNCTION__);
}
