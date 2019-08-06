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

#include "config.h"
#include "logger_init.h"
#include "config_parsers.h"
#include "on_leaving_scope.h"

#include "repositories/users_repository.h"

using namespace std;
using namespace lotr;

struct PerSocketData {
    uint32_t connection_id;
    uint32_t user_id;
};

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

    uWS::App().get("/register", [](uWS::HttpResponse<false> *res, uWS::HttpRequest *req) {
        res->writeStatus("200 OK");
        res->end("Hello world!");
    }).listen(3000, [&](auto *token) {
        if (token) {
            spdlog::info("[main] listening on \"{}:{}\"", config.address, config.port);
        }
    }).run();

    return 0;
}