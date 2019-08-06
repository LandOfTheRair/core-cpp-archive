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

#include "config_parsers.h"

#include <fstream>
#include <spdlog/spdlog.h>
#include <rapidjson/document.h>

using namespace lotr;
using namespace rapidjson;

optional<config> lotr::parse_env_file() {
    string env_contents;
    ifstream env("config.json");

    if(!env) {
        spdlog::error("[main] no config.json file found. Please make one.");
        return {};
    }

    env.seekg(0, ios::end);
    env_contents.resize(env.tellg());
    env.seekg(0, ios::beg);
    env.read(&env_contents[0], env_contents.size());
    env.close();

    spdlog::trace(R"([main] config.json file contents: {})", env_contents);

    Document d;
    d.Parse(env_contents.c_str());

    if (d.HasParseError() || !d.IsObject()) {
        spdlog::error("[main] deserialize config.json failed");
        return {};
    }

    if(!d.HasMember("DEBUG_LEVEL")) {
        spdlog::error("[main] deserialize config.json missing DEBUG_LEVEL");
        return {};
    }

    if(!d.HasMember("ADDRESS")) {
        spdlog::error("[main] deserialize config.json missing ADDRESS");
        return {};
    }

    if(!d.HasMember("PORT")) {
        spdlog::error("[main] deserialize config.json missing PORT");
        return {};
    }

    if(!d.HasMember("CONNECTION_STRING")) {
        spdlog::error("[main] deserialize config.json missing CONNECTION_STRING");
        return {};
    }

    config config;
    config.debug_level = d["DEBUG_LEVEL"].GetString();
    config.address = d["ADDRESS"].GetString();
    config.port = d["PORT"].GetInt();
    config.connection_string = d["CONNECTION_STRING"].GetString();

    return config;
}