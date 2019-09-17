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

#include <catch2/catch.hpp>
#include <messages/user_access/login_request.h>
#include <messages/user_access/login_response.h>
#include <messages/user_access/register_request.h>
#include <messages/user_access/play_character_request.h>
#include <messages/user_access/create_character_request.h>
#include <messages/user_access/create_character_response.h>
#include <messages/commands/move_request.h>
#include <messages/map_update_response.h>
#include <messages/generic_error_response.h>

#include <ecs/components.h>

using namespace std;
using namespace lotr;

#define SERDE(type, ...) type msg(__VA_ARGS__); \
            rapidjson::Document d; \
            auto ser = msg.serialize(); \
            d.Parse(ser.c_str(), ser.size()); \
            auto msg2 = type::deserialize(d);

TEST_CASE("message serialization tests") {

    // user access control

    SECTION("login request") {
        SERDE(login_request, "user", "pass");
        REQUIRE(msg.username == msg2->username);
        REQUIRE(msg.password == msg2->password);
    }

    SECTION("empty login response") {
        vector<message_player> players;
        SERDE(login_response, players);
        REQUIRE(msg.players.size() == msg2->players.size());
    }

    SECTION("login response") {
        vector<message_player> players;
        players.emplace_back("name", "map", 1, 2);
        players.emplace_back("name2", "map2", 3, 4);
        SERDE(login_response, players);
        REQUIRE(msg.players.size() == msg2->players.size());
        for(uint32_t i = 0; i < msg.players.size(); i++) {
            REQUIRE(msg.players[i].name == msg2->players[i].name);
            REQUIRE(msg.players[i].map_name == msg2->players[i].map_name);
            REQUIRE(msg.players[i].x == msg2->players[i].x);
            REQUIRE(msg.players[i].y == msg2->players[i].y);
        }
    }

    SECTION("register request") {
        SERDE(register_request, "user", "pass", "email");
        REQUIRE(msg.username == msg2->username);
        REQUIRE(msg.password == msg2->password);
        REQUIRE(msg.email == msg2->email);
    }

    SECTION("play character request") {
        SERDE(play_character_request, "character");
        REQUIRE(msg.name == msg2->name);
    }

    SECTION("play character request") {
        SERDE(create_character_request, "name", "sex", "allegiance", "baseclass");
        REQUIRE(msg.name == msg2->name);
        REQUIRE(msg.sex == msg2->sex);
        REQUIRE(msg.allegiance == msg2->allegiance);
        REQUIRE(msg.baseclass == msg2->baseclass);
    }

    SECTION("play character request") {
        vector<stat_component> player_stats;
        player_stats.emplace_back("hp", 10);
        player_stats.emplace_back("mp", 20);
        SERDE(create_character_response, "name", player_stats);
        REQUIRE(msg.name == msg2->name);
        REQUIRE(msg.stats.size() == msg2->stats.size());
        for(uint32_t i = 0; i < msg.stats.size(); i++) {
            REQUIRE(msg.stats[i].name == msg2->stats[i].name);
            REQUIRE(msg.stats[i].value == msg2->stats[i].value);
        }
    }

    // commands

    SECTION("move request") {
        SERDE(move_request, 2, 4);
        REQUIRE(msg.x == msg2->x);
        REQUIRE(msg.y == msg2->y);
    }

    // misc

    SECTION("map update response") {
        vector<character_component> npcs;
        character_component one;
        one.name = "test";
        one.sprite = 1;
        one.loc = make_tuple(2, 3);

        character_component two;
        two.name = "test2";
        two.sprite = 4;
        one.loc = make_tuple(5, 6);
        npcs.push_back(one);
        npcs.push_back(two);
        SERDE(map_update_response, npcs)
        REQUIRE(msg.npcs.size() == msg2->npcs.size());
        for(uint32_t i = 0; i < msg.npcs.size(); i++) {
            REQUIRE(msg.npcs[i].name == msg2->npcs[i].name);
            REQUIRE(msg.npcs[i].sprite == msg2->npcs[i].sprite);
            REQUIRE(msg.npcs[i].loc == msg2->npcs[i].loc);
        }
    }

    SECTION("generic error response") {
        SERDE(generic_error_response, "err", "name", "desc", true);
        REQUIRE(msg.error == msg2->error);
        REQUIRE(msg.pretty_error_name == msg2->pretty_error_name);
        REQUIRE(msg.pretty_error_name == msg2->pretty_error_name);
        REQUIRE(msg.clear_login_data == msg2->clear_login_data);
    }
}
