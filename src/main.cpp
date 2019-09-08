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
#include <chrono>

#include <message_handlers/login_handler.h>
#include <message_handlers/register_handler.h>

#include <entt/entt.hpp>
#include <asset_loading/load_map.h>
#include <asset_loading/load_item.h>
#include <asset_loading/load_npc.h>

#include "config.h"
#include "logger_init.h"
#include "config_parsers.h"


#include "repositories/users_repository.h"
#include "repositories/banned_users_repository.h"
#include "repositories/players_repository.h"
#include "working_directory_manipulation.h"
#include "per_socket_data.h"
#include "lotr_flat_map.h"
#include "random_helper.h"

#include "ai/default_ai.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"

using namespace std;
using namespace lotr;

atomic<uint64_t> connection_id_counter;
atomic<bool> quit{false};
atomic<uint64_t> npc_id_counter = 0;

void on_sigint(int sig) {
    quit = true;
}

struct uws_is_shit_struct {
    us_listen_socket_t *socket;
    uWS::Loop *loop;
};

optional<npc_component> create_npc(global_npc_component const &global_npc, spawner_script *script) {
    if(global_npc.sprite.empty()) {
        spdlog::error("global npc {}:{} has no sprites", global_npc.name, global_npc.npc_id);
        return {};
    }

    npc_component npc;
    npc.id = npc_id_counter++;
    npc.name = global_npc.name;
    npc.npc_id = global_npc.npc_id;
    npc.allegiance = global_npc.allegiance;
    npc.alignment = global_npc.alignment;
    npc.sex = global_npc.sex;
    npc.dir = global_npc.dir;
    npc.hostility = global_npc.hostility;
    npc.character_class = global_npc.character_class;
    npc.monster_class = global_npc.monster_class;
    npc.spawn_message = global_npc.spawn_message;
    npc.sfx = global_npc.sfx;
    npc.level = npc.highest_level = global_npc.level;

    npc.sprite = global_npc.sprite[lotr::random.generate_single(0, global_npc.sprite.size() - 1)];
    npc.skill_on_kill = global_npc.skill_on_kill;
    npc.sfx_max_chance = global_npc.sfx_max_chance;

    npc.spawner = script;

    for (auto const &stat : stats) {
        auto const random_stat_it = find_if(cbegin(global_npc.random_stats), cend(global_npc.random_stats),
                                            [&](random_stat_component const &rs) noexcept { return rs.name == stat; });

        if (random_stat_it != cend(global_npc.random_stats)) {
            npc.stats[stat] = lotr::random.generate_single(random_stat_it->min, random_stat_it->max);
        } else {
            auto const stat_it = find_if(cbegin(global_npc.stats), cend(global_npc.stats),
                                         [&](stat_component const &s) noexcept { return s.name == stat; });
            if (stat_it == cend(global_npc.stats)) {
                spdlog::error("Initializing spawn for global_npc {} failed, missing stat {}", global_npc.npc_id, stat);
                return {};
            }
            npc.stats[stat] = stat_it->value;
        }

        spdlog::trace("Added {}:{} to npc {}:{}", stat, npc.stats[stat], npc.id, npc.name);
    }

    spdlog::debug("Created npc {}:{}", npc.name, npc.npc_id);
    return npc;
}

void remove_dead_npcs(vector<npc_component> &npcs) {
    if(!npcs.empty()) {
        remove_if(begin(npcs), end(npcs), [&](npc_component &npc) noexcept { return npc.stats["hp"] <= 0; });
    }
}

void fill_spawners(vector<npc_component> &npcs, entt::registry &registry) {
    lotr_flat_map<uint32_t, tuple<uint32_t, spawner_script*>> spawner_npc_counter;

    for(auto &npc : npcs) {
        if(npc.stats["hp"] <= 0) {
            continue;
        }

        if(npc.spawner) {
            auto spawner_it = spawner_npc_counter.find(npc.spawner->id);

            if (spawner_it == end(spawner_npc_counter)) {
                get<0>(spawner_it->second)++;
            } else {
                spawner_npc_counter[npc.spawner->id] = make_tuple(1, npc.spawner);
            }
        }
    }

    for(auto &[k, v] : spawner_npc_counter) {
        if(get<0>(v) < get<1>(v)->max_creatures) {
            if(get<1>(v)->npc_ids.empty()) {
                continue;
            }

            auto random_npc_id = get<1>(v)->npc_ids[lotr::random.generate_single(0, get<1>(v)->npc_ids.size() - 1)].name;

            registry.view<global_npc_component>().each([&](global_npc_component const &global_npc) noexcept {
                if(global_npc.npc_id != random_npc_id) {
                    return;
                }

                auto npc = create_npc(global_npc, get<1>(v));

                if(npc) {
                    npcs.emplace_back(move(*npc));
                }
            });
        }
    }
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

    uint32_t item_count = 0;
    uint32_t npc_count = 0;
    uint32_t map_count = 0;
    uint32_t entity_count = 0;
    auto items_loading_start = chrono::system_clock::now();
    for(auto& p: filesystem::recursive_directory_iterator("assets/items")) {
        if(!p.is_regular_file() || quit) {
            continue;
        }

        auto items = load_global_items_from_file(p.path().string());

        for(auto &item: items) {
            auto new_entity = registry.create();
            registry.assign<global_item_component>(new_entity, move(item));
            item_count++;
        }
    }

    auto npcs_loading_start = chrono::system_clock::now();
    for(auto& p: filesystem::recursive_directory_iterator("assets/npcs")) {
        if(!p.is_regular_file() || quit) {
            continue;
        }

        auto npcs = load_global_npcs_from_file(p.path().string());

        for(auto &npc: npcs) {
            auto new_entity = registry.create();
            registry.assign<global_npc_component>(new_entity, move(npc));
            npc_count++;
        }
    }

    auto maps_loading_start = chrono::system_clock::now();
    for(auto& p: filesystem::recursive_directory_iterator("assets/maps")) {
        if(!p.is_regular_file() || quit) {
            continue;
        }

        auto map = load_map_from_file(p.path().string());

        if(!map) {
            continue;
        }

        for(uint32_t i = 0; i < 10; i++) {
            map->players.emplace_back();
        }

        auto new_entity = registry.create();
        registry.assign<map_component>(new_entity, move(map.value()));
        map_count++;
    }

    auto entity_spawning_start = chrono::system_clock::now();
    const string spawners_layer_name = "Spawners"s;
    registry.view<map_component>().each([&](map_component &m) noexcept {
        if(quit) {
            return;
        }

        auto spawners_layer = find_if(begin(m.layers), end(m.layers), [&](map_layer const &l) noexcept {return l.name == spawners_layer_name;}); // Case-sensitive. This will probably bite us in the ass later.
        if(spawners_layer == end(m.layers)) {
            spdlog::warn("No npc layer found for map {}", m.name);
            return;
        }

        for(auto &spawner_object : spawners_layer->objects) {
            if(spawner_object.gid == 0 || !spawner_object.script) {
                continue;
            }

            if(spawner_object.script->initial_spawn == 0 || spawner_object.script->max_creatures == 0 || spawner_object.script->npc_ids.empty()) {
                continue;
            }

            for(uint32_t i = 0; i < spawner_object.script->initial_spawn; i++) {
                spdlog::info("spawner_object {} has {} npc_ids", spawner_object.name, spawner_object.script->npc_ids.size());
                auto random_npc_id = spawner_object.script->npc_ids[lotr::random.generate_single(0, spawner_object.script->npc_ids.size() - 1)].name;
                registry.view<global_npc_component>().each([&](global_npc_component const &global_npc) noexcept {
                    if(global_npc.npc_id != random_npc_id) {
                        return;
                    }

                    auto npc = create_npc(global_npc, &spawner_object.script.value());

                    if(npc) {
                        m.npcs.emplace_back(move(*npc));
                        entity_count++;
                    }
                });
            }
        }
    });

    auto loading_end = chrono::system_clock::now();
    spdlog::info("[{}] {} items loaded in {} µs", __FUNCTION__, item_count, chrono::duration_cast<chrono::microseconds>(npcs_loading_start - items_loading_start).count());
    spdlog::info("[{}] {} npcs loaded in {} µs", __FUNCTION__, npc_count, chrono::duration_cast<chrono::microseconds>(maps_loading_start - npcs_loading_start).count());
    spdlog::info("[{}] {} maps loaded in {} µs", __FUNCTION__, map_count, chrono::duration_cast<chrono::microseconds>(entity_spawning_start - maps_loading_start).count());
    spdlog::info("[{}] {} entities spawned in {} µs", __FUNCTION__, entity_count, chrono::duration_cast<chrono::microseconds>(loading_end - entity_spawning_start).count());
    spdlog::info("[{}] everything loaded in {} µs", __FUNCTION__, chrono::duration_cast<chrono::microseconds>(loading_end - items_loading_start).count());

    vector<uint64_t> frame_times;
    auto next_tick = chrono::system_clock::now() + chrono::milliseconds(config.tick_length);
    auto next_log_tick_times = chrono::system_clock::now() + chrono::seconds(1);
    uint32_t tick_counter = 0;
    while (!quit) {
        auto now = chrono::system_clock::now();
        if(now < next_tick) {
            this_thread::sleep_until(next_tick);
        }
        spdlog::trace("[{}] starting tick", __FUNCTION__);
        auto tick_start = chrono::system_clock::now();
        auto map_view = registry.view<map_component>();
        for(auto m_entity : map_view) {
            map_component &m = map_view.get(m_entity);
            lotr_player_location_map player_location_map;

            for (auto &player : m.players) {
                auto loc_tuple = make_tuple(player.x, player.y);
                auto existing_players = player_location_map.find(loc_tuple);

                if (existing_players == end(player_location_map)) {
                    player_location_map[loc_tuple] = vector<pc_component *>{&player};
                } else {
                    existing_players->second.push_back(&player);
                }

                player.fov = compute_fov_restrictive_shadowcasting(m, player.x, player.y, true);
            }

            remove_dead_npcs(m.npcs);
            fill_spawners(m.npcs, registry);
        }

        auto tick_end = chrono::system_clock::now();
        frame_times.push_back(chrono::duration_cast<chrono::microseconds>(tick_end - tick_start).count());
        next_tick += chrono::milliseconds(config.tick_length);
        tick_counter++;

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
    shit_uws.loop->defer([&shit_uws] {
        us_listen_socket_close(0, shit_uws.socket);
    });
    uws_thread.join();
    spdlog::warn("[{}] uws thread stopped", __FUNCTION__);

    return 0;
}
#pragma clang diagnostic pop
