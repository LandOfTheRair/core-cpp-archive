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

#pragma once

namespace lotr {
    struct db_location {
        uint64_t id;
        string map_name;
        uint32_t x;
        uint32_t y;

        db_location() : id(), map_name(), x(), y() {}
        db_location(uint64_t id, string map_name, uint32_t x, uint32_t y) : id(id), map_name(move(map_name)), x(x), y(y) {}
    };

    struct user {
        uint64_t id;
        string username;
        string password;
        string email;
        uint16_t login_attempts;
        string verification_code;
        uint16_t max_characters;
        uint16_t is_game_master;

        user() : id(), username(), password(), email(), login_attempts(), verification_code(), max_characters(), is_game_master() {}
        user(uint64_t id, string username, string password, string email, uint16_t login_attempts, string verification_code, uint16_t no_of_players, uint16_t admin)
        : id(id), username(move(username)), password(move(password)), email(move(email)), login_attempts(login_attempts), verification_code(move(verification_code)), max_characters(no_of_players), is_game_master(admin) {}
    };

    struct banned_user {
        uint64_t id;
        string ip;
        optional<user> _user;
        optional<chrono::system_clock::time_point> until;

        banned_user() : id(), ip(), _user(), until() {}
        banned_user(uint64_t id, string ip, optional<user> _user, optional<chrono::system_clock::time_point> until)
        : id(id), ip(move(ip)), _user(move(_user)), until(until) {}
    };

    struct player_stat {
        uint64_t id;
        string name;
        uint64_t value;

        player_stat() : id(), name(), value() {}
        player_stat(uint64_t id, string name, uint64_t value) : id(id), name(move(name)), value(value) {}
    };

    struct player_item {
        uint64_t id;
        string name;

        player_item() : id(), name() {}
        player_item(uint64_t id, string name) : id(id), name(move(name)) {}
    };

    struct db_character {
        uint64_t id;
        uint64_t user_id;
        uint64_t location_id;
        string name;
        string allegiance;
        string gender;
        string alignment;
        string _class;
        optional<db_location> loc;
        vector<player_stat> stats;
        vector<player_item> items;

        db_character() : id(), user_id(), location_id(), name(), loc(), stats(), items() {}
        db_character(uint64_t id, uint64_t user_id, uint64_t location_id, string name, string allegiance, string gender, string alignment, string _class, optional<db_location> loc, vector<player_stat> stats, vector<player_item> items)
        : id(id), user_id(user_id), location_id(location_id), name(move(name)), allegiance(move(allegiance)), gender(move(gender)), alignment(move(alignment)), _class(move(_class)), loc(move(loc)), stats(move(stats)), items(move(items)) {}
    };
}

