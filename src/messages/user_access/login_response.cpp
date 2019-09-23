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

#include "login_response.h"
#include <spdlog/spdlog.h>
#include <rapidjson/writer.h>

using namespace lotr;
using namespace rapidjson;

string const login_response::type = "Auth:login_response";

login_response::login_response(vector<message_player> players, vector<account_object> online_users, string username, string email, string motd) noexcept :
    players(move(players)), online_users(move(online_users)), username(move(username)), email(move(email)), motd(move(motd)) {
}

string login_response::serialize() const {
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);

    writer.StartObject();

    writer.String("type");
    writer.String(type.c_str(), type.size());

    writer.String("username");
    writer.String(username.c_str(), username.size());

    writer.String("email");
    writer.String(email.c_str(), email.size());

    writer.String("motd");
    writer.String(motd.c_str(), motd.size());

    writer.String("players");
    writer.StartArray();

    for(auto& player : players) {
        writer.StartObject();

        writer.String("name");
        writer.String(player.name.c_str(), player.name.size());

        writer.String("map_name");
        writer.String(player.map_name.c_str(), player.map_name.size());

        writer.String("x");
        writer.Int(player.x);

        writer.String("y");
        writer.Int(player.y);

        writer.EndObject();
    }

    writer.EndArray();

    writer.String("online_users");
    writer.StartArray();

    for(auto& user : online_users) {
        writer.StartObject();

        writer.String("is_game_master");
        writer.Bool(user.is_game_master);

        writer.String("is_tester");
        writer.Bool(user.is_tester);

        writer.String("has_done_trial");
        writer.Bool(user.has_done_trial);

        writer.String("trial_ends_unix_timestamp");
        writer.Uint64(user.trial_ends_unix_timestamp);

        writer.String("subscription_tier");
        writer.Uint(user.subscription_tier);

        writer.String("username");
        writer.String(user.username.c_str(), user.username.size());

        writer.EndObject();
    }

    writer.EndArray();

    writer.EndObject();
    return sb.GetString();
}

optional<login_response> login_response::deserialize(rapidjson::Document const &d) {
    if (!d.HasMember("type") || !d.HasMember("players") || !d.HasMember("username") || !d.HasMember("email") || !d.HasMember("motd")) {
        spdlog::warn("[login_response] deserialize failed");
        return nullopt;
    }

    if(d["type"].GetString() != type) {
        spdlog::warn("[login_response] deserialize failed wrong type");
        return nullopt;
    }

    vector<message_player> players;
    auto &players_array = d["players"];
    if(!players_array.IsArray()) {
        spdlog::warn("[login_response] deserialize failed");
        return nullopt;
    }

    vector<account_object> online_users;
    auto &online_users_array = d["online_users"];
    if(!online_users_array.IsArray()) {
        spdlog::warn("[login_response] deserialize failed");
        return nullopt;
    }

    for(SizeType i = 0; i < online_users_array.Size(); i++) {
        if (!online_users_array[i].IsObject() ||
            !online_users_array[i].HasMember("is_game_master") ||
            !online_users_array[i].HasMember("is_tester") ||
            !online_users_array[i].HasMember("has_done_trial") ||
            !online_users_array[i].HasMember("trial_ends_unix_timestamp") ||
            !online_users_array[i].HasMember("subscription_tier") ||
            !online_users_array[i].HasMember("username")) {
            spdlog::warn("[login_response] deserialize failed");
            return nullopt;
        }
        online_users.emplace_back(online_users_array[i]["is_game_master"].GetBool(), online_users_array[i]["is_tester"].GetBool(), online_users_array[i]["has_done_trial"].GetBool(),
                online_users_array[i]["trial_ends_unix_timestamp"].GetUint64(), online_users_array[i]["subscription_tier"].GetUint(), online_users_array[i]["username"].GetString());
    }

    for(SizeType i = 0; i < players_array.Size(); i++) {
        if (!players_array[i].IsObject() ||
            !players_array[i].HasMember("name") ||
            !players_array[i].HasMember("map_name") ||
            !players_array[i].HasMember("x") ||
            !players_array[i].HasMember("y")) {
            spdlog::warn("[login_response] deserialize failed");
            return nullopt;
        }
        players.emplace_back(players_array[i]["name"].GetString(), players_array[i]["map_name"].GetString(), players_array[i]["x"].GetInt(), players_array[i]["y"].GetInt());
    }

    return login_response(players, online_users, d["username"].GetString(), d["email"].GetString(), d["motd"].GetString());
}
