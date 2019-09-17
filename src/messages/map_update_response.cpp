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

#include "map_update_response.h"
#include <spdlog/spdlog.h>
#include <rapidjson/writer.h>
#include <ecs/components.h>

using namespace lotr;
using namespace rapidjson;

string const map_update_response::type = "Game:map_update";

map_update_response::map_update_response(vector<character_component> npcs) noexcept : npcs(move(npcs)) {

}

string map_update_response::serialize() const {
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);

    writer.StartObject();

    writer.String("type");
    writer.String(type.c_str(), type.size());

    writer.String("npcs");

    writer.StartArray();
    for(auto &npc: npcs) {
        writer.StartObject();

        writer.String("name");
        writer.String(npc.name.c_str(), npc.name.size());

        writer.String("sprite");
        writer.Uint(npc.sprite);

        writer.String("x");
        writer.Uint(get<0>(npc.loc));

        writer.String("y");
        writer.Uint(get<1>(npc.loc));

        writer.EndObject();
    }
    writer.EndArray();

    writer.EndObject();
    return sb.GetString();
}

optional<map_update_response> map_update_response::deserialize(rapidjson::Document const &d) {
    if (!d.HasMember("type") || !d.HasMember("npcs")) {
        spdlog::warn("[map_update_response] deserialize failed");
        return nullopt;
    }

    if(d["type"].GetString() != type) {
        spdlog::warn("[map_update_response] deserialize failed wrong type");
        return nullopt;
    }

    vector<character_component> players;
    auto &npcs_array = d["npcs"];
    if(!npcs_array.IsArray()) {
        spdlog::warn("[map_update_response] deserialize failed");
        return nullopt;
    }

    for(SizeType i = 0; i < npcs_array.Size(); i++) {
        if (!npcs_array[i].IsObject() ||
            !npcs_array[i].HasMember("name") ||
            !npcs_array[i].HasMember("sprite") ||
            !npcs_array[i].HasMember("x") ||
            !npcs_array[i].HasMember("y")) {
            spdlog::warn("[map_update_response] deserialize failed");
            return nullopt;
        }

        npc_component npc;
        npc.name = npcs_array[i]["name"].GetString();
        npc.sprite = npcs_array[i]["sprite"].GetInt();
        npc.loc = make_tuple(npcs_array[i]["x"].GetInt(), npcs_array[i]["y"].GetInt());

        players.push_back(npc);
    }

    return map_update_response(players);
}
