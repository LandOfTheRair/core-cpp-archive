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

#include "create_character_response.h"
#include <spdlog/spdlog.h>
#include <rapidjson/writer.h>
#include <ecs/components.h>

using namespace lotr;
using namespace rapidjson;

string const create_character_response::type = "Game:create_character_response";

create_character_response::create_character_response(string name, vector<stat_component> stats) noexcept : name(move(name)), stats(move(stats)) {

}

string create_character_response::serialize() const {
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);

    writer.StartObject();

    writer.String("type");
    writer.String(type.c_str(), type.size());

    writer.String("name");
    writer.String(name.c_str(), name.size());

    writer.String("stats");

    writer.StartArray();
    for(auto &stat : stats) {
        writer.StartObject();
        writer.String("name");
        writer.String(stat.name.c_str(), name.size());

        writer.String("value");
        writer.Int64(stat.value);
        writer.EndObject();
    }
    writer.EndArray();

    writer.EndObject();
    return sb.GetString();
}

optional<create_character_response> create_character_response::deserialize(rapidjson::Document const &d) {
    if (!d.HasMember("type") || !d.HasMember("name") || ! d.HasMember("stats")) {
        spdlog::warn("[create_character_response] deserialize failed");
        return nullopt;
    }

    if(d["type"].GetString() != type) {
        spdlog::warn("[create_character_response] deserialize failed wrong type");
        return nullopt;
    }

    vector<stat_component> stats;
    auto &stats_array = d["stats"];
    if(!stats_array.IsArray()) {
        spdlog::warn("[create_character_response] deserialize failed");
        return nullopt;
    }

    for(SizeType i = 0; i < stats_array.Size(); i++) {
        if (!stats_array[i].IsObject() ||
            !stats_array[i].HasMember("name") ||
            !stats_array[i].HasMember("value")) {
            spdlog::warn("[create_character_response] deserialize failed");
            return nullopt;
        }
        stats.emplace_back(stats_array[i]["name"].GetString(), stats_array[i]["value"].GetInt64());
    }

    return create_character_response(d["name"].GetString(), stats);
}
