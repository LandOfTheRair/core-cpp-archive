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

#include <libusockets.h>
#include <Loop.h>
#include <config.h>
#include <database/database_pool.h>
#include <lotr_flat_map.h>
#include <WebSocket.h>
#include <readerwriterqueue.h>

#include <game_queue_messages/messages.h>

namespace lotr {
    struct uws_is_shit_struct {
        us_listen_socket_t *socket;
        uWS::Loop *loop;
    };

    extern lotr_flat_map<uint64_t, uWS::WebSocket<false, true> *> user_connections;
    extern moodycamel::ReaderWriterQueue<unique_ptr<queue_message>> game_loop_queue;

    void run_uws(config &config, shared_ptr<database_pool> pool, uws_is_shit_struct &shit_uws, atomic<bool> &quit);
}
