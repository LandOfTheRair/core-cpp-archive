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
#include <climits>

#include "../src/config.h"
#include "../src/config_parsers.h"
#include "benchmark_helpers/startup_helper.h"
#include "../src/working_directory_manipulation.h"

using namespace std;
using namespace lotr;

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


}