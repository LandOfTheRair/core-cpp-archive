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

namespace lotr {
    template<typename pool_T, typename transaction_T>
    class stats_repository  {
    public:
        explicit stats_repository(shared_ptr<pool_T> database_pool);

        unique_ptr<transaction_T> create_transaction();

        void insert(character_stat &stat, unique_ptr<transaction_T> const &transaction) const;
        void update(character_stat const &stat, unique_ptr<transaction_T> const &transaction) const;
        optional<character_stat> get(uint64_t id, unique_ptr<transaction_T> const &transaction) const;
        vector<character_stat> get_by_character_id(uint64_t character_id, unique_ptr<transaction_T> const &transaction) const;
    private:
        shared_ptr<pool_T> _database_pool;
    };
}
