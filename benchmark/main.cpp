/*
    Land of the Rair
    Copyright (C) 2019  Michael de Lang

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
#include <filesystem>
#include <chrono>
#include <game_logic/censor_sensor.h>
#include <sodium.h>

#include "../src/config.h"
#include "../src/config_parsers.h"
#include "benchmark_helpers/startup_helper.h"
#include "../src/working_directory_manipulation.h"
#include "game_logic/fov.h"
#include "../src/asset_loading/load_map.h"
#include <game_logic/a_star.h>

using namespace std;
using namespace lotr;

void bench_censor_sensor() {
    censor_sensor s("assets/profanity_locales/en.json");

    auto start = chrono::system_clock::now();

    for(int i = 0; i < 1'000'000; i++) {
        s.is_profane("this is bollocks");
    }

    auto end = chrono::system_clock::now();

    spdlog::info("is_profane {:n} µs", chrono::duration_cast<chrono::microseconds>(end-start).count());
}

void bench_fov(map_component const &m) {
    auto start = chrono::system_clock::now();

    for(int i = 0; i < 1'000'000; i++) {
        compute_fov_restrictive_shadowcasting(m, 4, 4, false);
    }

    auto end = chrono::system_clock::now();

    spdlog::info("compute_fov_restrictive_shadowcasting {:n} µs", chrono::duration_cast<chrono::microseconds>(end-start).count());
}

char hashed_password[crypto_pwhash_STRBYTES];
string test_pass = "very_secure_password";
#define crypto_pwhash_argon2id_MEMLIMIT_rair 33554432U

void bench_hashing() {
    auto start = chrono::system_clock::now();

    if (crypto_pwhash_str(hashed_password,
                          test_pass.c_str(),
                          test_pass.length(),
                          crypto_pwhash_argon2id_OPSLIMIT_MODERATE,
                          crypto_pwhash_argon2id_MEMLIMIT_rair) != 0) {
        spdlog::error("out of memory?");
        return;
    }

    auto end = chrono::system_clock::now();

    spdlog::info("hashing {:n} µs", chrono::duration_cast<chrono::microseconds>(end-start).count());
}

void bench_hash_verify() {
    auto start = chrono::system_clock::now();

    if (crypto_pwhash_str_verify(hashed_password, test_pass.c_str(), test_pass.length()) != 0) {
        spdlog::error("Hash should verify");
    }

    auto end = chrono::system_clock::now();

    spdlog::info("hashing verify {:n} µs", chrono::duration_cast<chrono::microseconds>(end-start).count());
}

void bench_a_star(map_component const &m) {
    auto start = chrono::system_clock::now();
    auto start_loc = make_tuple(10, 10);
    auto goal_loc = make_tuple(25, 25);

    for(int i = 0; i < 100'000; i++) {
        a_star_path(m, start_loc, goal_loc);
    }

    auto end = chrono::system_clock::now();

    spdlog::info("a star {:n} µs", chrono::duration_cast<chrono::microseconds>(end-start).count());
}

int main(int argc, char **argv) {
    set_cwd(get_selfpath());

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
    db_pool = make_shared<database_pool>();
    db_pool->create_connections(config.connection_string, 2);

    if(sodium_init() != 0) {
        spdlog::error("sodium init failure");
        return 1;
    }

    entt::registry registry;
    auto m = load_map_from_file("assets/maps/antania/DedlaenMaze.json", registry).value();

    bench_censor_sensor();
    bench_fov(m);
    bench_hashing();
    bench_hash_verify();
    bench_a_star(m);

}
