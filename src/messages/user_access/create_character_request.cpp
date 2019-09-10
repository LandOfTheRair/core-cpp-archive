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

#include "create_character_request.h"
#include <spdlog/spdlog.h>
#include <rapidjson/writer.h>

using namespace lotr;
using namespace rapidjson;

string const create_character_request::type = "Game:create_character";

create_character_request::create_character_request(string name, string sex, string allegiance, string baseclass) noexcept
    : name(move(name)), sex(move(sex)), allegiance(move(allegiance)), baseclass(move(baseclass)) {

}

string create_character_request::serialize() const {
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);

    writer.StartObject();

    writer.String("type");
    writer.String(type.c_str(), type.size());

    writer.String("name");
    writer.String(name.c_str(), name.size());

    writer.String("sex");
    writer.String(sex.c_str(), sex.size());

    writer.String("allegiance");
    writer.String(allegiance.c_str(), allegiance.size());

    writer.String("baseclass");
    writer.String(baseclass.c_str(), baseclass.size());

    writer.EndObject();
    return sb.GetString();
}

optional<create_character_request> create_character_request::deserialize(rapidjson::Document const &d) {
    if (!d.HasMember("type") || !d.HasMember("name") || ! d.HasMember("sex") || ! d.HasMember("allegiance") || ! d.HasMember("baseclass")) {
        spdlog::warn("[create_character_request] deserialize failed");
        return nullopt;
    }

    if(d["type"].GetString() != type) {
        spdlog::warn("[create_character_request] deserialize failed wrong type");
        return nullopt;
    }

    return create_character_request(d["name"].GetString(), d["sex"].GetString(), d["allegiance"].GetString(), d["baseclass"].GetString());
}
