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
#include <atomic>
#include <functional>
#include <csignal>
#include <chrono>
#include <filesystem>

#include <entt/entt.hpp>
#include <asset_loading/load_assets.h>
#include <game_logic/logic_helpers.h>
#include <readerwriterqueue.h>
#include <sodium.h>
#include <messages/map_update_response.h>
#include <game_queue_message_handlers/player_enter_handler.h>
#include <game_queue_message_handlers/player_leave_handler.h>
#include <game_queue_message_handlers/commands/player_move_handler.h>
#include <range/v3/all.hpp>

#include "config.h"
#include "logger_init.h"
#include "config_parsers.h"

#include "repositories/users_repository.h"
#include "repositories/banned_users_repository.h"
#include "repositories/characters_repository.h"
#include "working_directory_manipulation.h"

#include "ai/default_ai.h"
#include "uws_thread.h"
using namespace std;
using namespace lotr;

atomic<bool> quit{false};

void on_sigint(int sig) {
    quit = true;
}

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
        spdlog::error("[{}] config.json file is malformed json.", __FUNCTION__);
        return 1;
    }

    if(!filesystem::exists("logs")) {
        if(!filesystem::create_directory("logs")) {
            spdlog::error("[{}] Fatal error creating logs directory", __FUNCTION__);
            return 1;
        }
    }

    reconfigure_logger(config);

    if(sodium_init() != 0) {
        spdlog::error("[{}] sodium init failure", __FUNCTION__);
        return 1;
    }

    auto pool = make_shared<database_pool>();
    pool->create_connections(config.connection_string, 1);

    users_repository<database_pool, database_transaction> user_repo(pool);
    banned_users_repository<database_pool, database_transaction> banned_user_repo(pool);
    characters_repository<database_pool, database_transaction> player_repo(pool);
    uws_is_shit_struct shit_uws{}; // The documentation in uWS is appalling and the attitude the guy has is impossible to deal with. Had to search the issues of the github to find a method to close/stop uWS.

    auto uws_thread = thread([&config, pool, &shit_uws] {
        run_uws(config, pool, shit_uws, quit);
    });

    entt::registry registry;

    load_assets(registry, quit);

    outward_queues per_player_outward_queue;
    vector<uint64_t> frame_times;
    auto next_tick = chrono::system_clock::now() + chrono::milliseconds(config.tick_length);
    auto next_log_tick_times = chrono::system_clock::now() + chrono::seconds(1);
    uint32_t tick_counter = 0;

    lotr_flat_map<uint32_t, function<void(queue_message*, entt::registry&, outward_queues&)>> game_queue_message_router;
    game_queue_message_router.emplace(player_enter_message::_type, handle_player_enter_message);
    game_queue_message_router.emplace(player_leave_message::_type, handle_player_leave_message);
    game_queue_message_router.emplace(player_move_message::_type, handle_player_move_message);

    while (!quit) {
        auto now = chrono::system_clock::now();
        if(now < next_tick) {
            this_thread::sleep_until(next_tick);
        }
        spdlog::trace("[{}] starting tick", __FUNCTION__);
        auto tick_start = chrono::system_clock::now();

        auto map_view = registry.view<map_component>();

        {
            unique_ptr<queue_message> msg;
            while (game_loop_queue.try_dequeue(msg)) {
                spdlog::trace("[{}] got game loop msg with type {}", __FUNCTION__, msg->type);
                game_queue_message_router[msg->type](msg.get(), registry, per_player_outward_queue);
            }
        }


        for(auto m_entity : map_view) {
            map_component &m = map_view.get(m_entity);
            lotr_player_location_map player_location_map;

            for (auto &player : m.players) {
                auto existing_players = player_location_map.find(player.loc);

                if (existing_players == end(player_location_map)) {
                    player_location_map[player.loc] = vector<pc_component *>{&player};
                } else {
                    existing_players->second.push_back(&player);
                }

                player.fov = compute_fov_restrictive_shadowcasting(m, player.loc, true);

                auto min_x = max(0u, get<0>(player.loc) - fov_max_distance);
                auto min_y = max(0u, get<1>(player.loc) - fov_max_distance);
                auto max_x = min(m.width, get<0>(player.loc) + fov_max_distance);
                auto max_y = min(m.height, get<1>(player.loc) + fov_max_distance);

                auto visible_npcs = m.npcs | ranges::views::filter([&](npc_component const &npc){ return is_visible(player.loc, npc.loc, player.fov, min_x, max_x, min_y, max_y); });
                auto visible_pcs = m.players | ranges::views::filter([&](pc_component const &pc){ return is_visible(player.loc, pc.loc, player.fov, min_x, max_x, min_y, max_y) && player.connection_id != pc.connection_id; });
                vector<character_component> cs;

                for(auto &npc : visible_npcs) {
                    cs.push_back(npc);
                }

                for(auto &pc : visible_pcs) {
                    cs.push_back(pc);
                }

                spdlog::trace("[{}] Enqueued map update response with {} visible npcs", __FUNCTION__, cs.size());
                per_player_outward_queue[player.connection_id].enqueue(make_unique<map_update_response>(cs));
            }

            remove_dead_npcs(m.npcs);
            fill_spawners(m, m.npcs, registry);

            for(auto &npc : m.npcs) {
                run_ai_on(npc, m, player_location_map);
            }
        }

        auto tick_end = chrono::system_clock::now();
        frame_times.push_back(chrono::duration_cast<chrono::microseconds>(tick_end - tick_start).count());
        next_tick += chrono::milliseconds(config.tick_length);
        tick_counter++;

        // TODO if moving inner loop to function, we get a double free. I don't understand why.
        if(config.use_ssl) {
            shit_uws.loop->defer([&] {
                for(auto &[conn_id, q] : per_player_outward_queue) {
                    auto conn = user_ssl_connections.find(conn_id);
                    if (conn != end(user_ssl_connections)) {
                        unique_ptr<message> msg;
                        while (q.try_dequeue(msg)) {
                            conn->second->send(msg->serialize(), uWS::OpCode::TEXT, true);
                        }
                    }
                }
            });
        } else {
            shit_uws.loop->defer([&] {
                for(auto &[conn_id, q] : per_player_outward_queue) {
                    auto conn = user_connections.find(conn_id);
                    if (conn != end(user_connections)) {
                        unique_ptr<message> msg;
                        while (q.try_dequeue(msg)) {
                            conn->second->send(msg->serialize(), uWS::OpCode::TEXT, true);
                        }
                    }
                }
            });
        }


        if(config.log_tick_times && tick_end > next_log_tick_times) {
            spdlog::info("[{}] ticks {} - frame times max/avg/min: {} / {} / {} µs", __FUNCTION__, tick_counter,
                         *max_element(begin(frame_times), end(frame_times)), accumulate(begin(frame_times), end(frame_times), 0ul) / frame_times.size(),
                         *min_element(begin(frame_times), end(frame_times)));
            frame_times.clear();
            next_log_tick_times += chrono::seconds(1);
            tick_counter = 0;
        }
    }

    spdlog::warn("[{}] quitting program", __FUNCTION__);
    if(config.use_ssl) {
        shit_uws.loop->defer([&shit_uws] {
            for (auto &[conn_id, ws] : user_ssl_connections) {
                ws->end(0);
            }

            us_listen_socket_close(1, shit_uws.socket);
        });
    } else {
        shit_uws.loop->defer([&shit_uws] {
            for (auto &[conn_id, ws] : user_connections) {
                ws->end(0);
            }

            us_listen_socket_close(0, shit_uws.socket);
        });
    }
    uws_thread.join();
    spdlog::warn("[{}] uws thread stopped", __FUNCTION__);

    return 0;
}
