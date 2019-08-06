/*
    lotr_backend
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

#include "register_request.h"
#include <spdlog/spdlog.h>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>

using namespace lotr;
using namespace rapidjson;

register_request::register_request(string username, string password) noexcept : username(move(username)), password(move(password)) {

}

string register_request::serialize() const {
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);

    writer.StartObject();

    writer.String("username");
    writer.String(username.c_str(), username.size());

    writer.String("password");
    writer.String(password.c_str(), password.size());

    writer.EndObject();
    return sb.GetString();
}

optional<register_request> register_request::deserialize(string const &data) {
    if(data.empty() || data.length() < 4) {
        spdlog::warn("[register_request] deserialize encountered empty buffer");
        return nullopt;
    }

    Document d;
    d.Parse(data.c_str());

    if (d.HasParseError() || !d.IsObject() || !d.HasMember("username") || ! d.HasMember("password")) {
        spdlog::warn("[register_request] deserialize failed");
        return nullopt;
    }

    return register_request(d["username"].GetString(), d["password"].GetString());
}
