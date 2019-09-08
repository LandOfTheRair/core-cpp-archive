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

#include "play_character_request.h"
#include <spdlog/spdlog.h>
#include <rapidjson/writer.h>

using namespace lotr;
using namespace rapidjson;

play_character_request::play_character_request(string name) noexcept : name(move(name)) {

}

string play_character_request::serialize() const {
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);

    writer.StartObject();

    writer.String("name");
    writer.String(name.c_str(), name.size());

    writer.EndObject();
    return sb.GetString();
}

optional<play_character_request> play_character_request::deserialize(rapidjson::Document const &d) {
    if (!d.HasMember("name")) {
        spdlog::warn("[play_character_request] deserialize failed");
        return nullopt;
    }

    return play_character_request(d["name"].GetString());
}
