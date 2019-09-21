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
#include <spdlog/spdlog.h>
#include "../test_helpers/startup_helper.h"
#include <message_handlers/user_access/register_handler.h>
#include <messages/user_access/register_request.h>
#include <messages/generic_error_response.h>

using namespace std;
using namespace lotr;

class custom_websocket : public uWS::WebSocket<false, true> {
public:
    bool send(std::string_view message, uWS::OpCode opCode = uWS::OpCode::BINARY, bool compress = false) {
        sent_message = message;

        return true;
    }

    string sent_message;
};

TEST_CASE("register handler tests") {
    SECTION("Prohibit too short usernames") {
        string message = register_request("a", "okay_password", "an_email").serialize();
        per_socket_data user_data;
        moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> q;
        custom_websocket ws;

        rapidjson::Document d;
        d.Parse(&message[0], message.size());

        handle_register<false>(&ws, uWS::OpCode::TEXT, d, db_pool, &user_data, q);

        d.Parse(&ws.sent_message[0], ws.sent_message.size());
        auto new_msg = generic_error_response::deserialize(d);
        REQUIRE(new_msg);
        REQUIRE(new_msg->error == "Usernames needs to be at least 2 characters and at most 20 characters");
    }

    SECTION("Prohibit too short usernames utf8") {
        string message = register_request("漢", "okay_password", "an_email").serialize();
        per_socket_data user_data;
        moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> q;
        custom_websocket ws;

        rapidjson::Document d;
        d.Parse(&message[0], message.size());

        handle_register<false>(&ws, uWS::OpCode::TEXT, d, db_pool, &user_data, q);

        d.Parse(&ws.sent_message[0], ws.sent_message.size());
        auto new_msg = generic_error_response::deserialize(d);
        REQUIRE(new_msg);
        REQUIRE(new_msg->error == "Usernames needs to be at least 2 characters and at most 20 characters");
    }

    SECTION("Prohibit too long usernames") {
        string message = register_request("aalishdiquwhgebilugfhkjsdhasdasd", "okay_password", "an_email").serialize();
        per_socket_data user_data;
        moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> q;
        custom_websocket ws;

        rapidjson::Document d;
        d.Parse(&message[0], message.size());

        handle_register<false>(&ws, uWS::OpCode::TEXT, d, db_pool, &user_data, q);

        d.Parse(&ws.sent_message[0], ws.sent_message.size());
        auto new_msg = generic_error_response::deserialize(d);
        REQUIRE(new_msg);
        REQUIRE(new_msg->error == "Usernames needs to be at least 2 characters and at most 20 characters");
    }

    SECTION("Prohibit too short password") {
        string message = register_request("ab", "shortpw", "an_email").serialize();
        per_socket_data user_data;
        moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> q;
        custom_websocket ws;

        rapidjson::Document d;
        d.Parse(&message[0], message.size());

        handle_register<false>(&ws, uWS::OpCode::TEXT, d, db_pool, &user_data, q);

        d.Parse(&ws.sent_message[0], ws.sent_message.size());
        auto new_msg = generic_error_response::deserialize(d);
        REQUIRE(new_msg);
        REQUIRE(new_msg->error == "Password needs to be at least 8 characters");
    }

    SECTION("Prohibit too short password utf8") {
        string message = register_request("ab", "漢字漢字漢字", "an_email").serialize();
        per_socket_data user_data;
        moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> q;
        custom_websocket ws;

        rapidjson::Document d;
        d.Parse(&message[0], message.size());

        handle_register<false>(&ws, uWS::OpCode::TEXT, d, db_pool, &user_data, q);

        d.Parse(&ws.sent_message[0], ws.sent_message.size());
        auto new_msg = generic_error_response::deserialize(d);
        REQUIRE(new_msg);
        REQUIRE(new_msg->error == "Password needs to be at least 8 characters");
    }
}
