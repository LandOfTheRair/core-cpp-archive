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

#include "register_request.h"
#include <spdlog/spdlog.h>
#include <rapidjson/writer.h>

using namespace lotr;
using namespace rapidjson;

string const register_request::type = "Auth:register";

register_request::register_request(string username, string password, string email) noexcept
 : username(move(username)), password(move(password)), email(move(email)) {

}

string register_request::serialize() const {
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);

    writer.StartObject();

    writer.String(KEY_STRING("type"));
    writer.String(type.c_str(), type.size());

    writer.String(KEY_STRING("username"));
    writer.String(username.c_str(), username.size());

    writer.String(KEY_STRING("password"));
    writer.String(password.c_str(), password.size());

    writer.String(KEY_STRING("email"));
    writer.String(email.c_str(), email.size());

    writer.EndObject();
    return sb.GetString();
}

optional<register_request> register_request::deserialize(rapidjson::Document const &d) {
    if (!d.HasMember("type") || !d.HasMember("username") || ! d.HasMember("password") || ! d.HasMember("email")) {
        spdlog::warn("[register_request] deserialize failed");
        return nullopt;
    }

    if(d["type"].GetString() != type) {
        spdlog::warn("[register_request] deserialize failed wrong type");
        return nullopt;
    }

    return register_request(d["username"].GetString(), d["password"].GetString(), d["email"].GetString());
}
