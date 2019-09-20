/*
    RealmOfAesirWorld
    Copyright (C) 2017  Michael de Lang

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

#include <string>
#include <memory>
#include <optional>
#include <database/database_pool.h>
#include <database/database_transaction.h>
#include "models.h"

using namespace std;

namespace lotr {
    enum class included_tables : int
    {
        none,
        stats,
        location,
        items,
        all
    };

    template<typename pool_T, typename transaction_T>
    class characters_repository {
    public:
        explicit characters_repository(shared_ptr<pool_T> database_pool);

        unique_ptr<transaction_T> create_transaction();

        bool insert(db_character &plyr, unique_ptr<transaction_T> const &transaction) const;
        bool insert_or_update_player(db_character &plyr, unique_ptr<transaction_T> const &transaction) const;
        void update_player(db_character const &plyr, unique_ptr<transaction_T> const &transaction) const;
        optional<db_character> get_player(string const &name, included_tables includes, unique_ptr<transaction_T> const &transaction) const;
        optional<db_character> get_player(uint64_t id, included_tables includes, unique_ptr<transaction_T> const &transaction) const;
        vector<db_character> get_by_user_id(uint64_t user_id, included_tables includes, unique_ptr<transaction_T> const &transaction) const;
    private:
        shared_ptr<pool_T> _database_pool;
    };
}
