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

#include <robin_hood.h>
#include <xxhash.h>
#include <string>

using namespace std;

namespace lotr {
    template <typename ... Ts>
    constexpr size_t tuple_sum_size(tuple<Ts...> const &)
    {
        return (sizeof(Ts) + ...);
    }

    template<class Key>
    class xxhash_function;

    template<>
    class xxhash_function<string>
    {
    public:
        size_t operator()(string const &key) const
        {
            return XXH3_64bits(key.c_str(), key.size());
        }

        size_t operator()(string_view const &key) const
        {
            return XXH3_64bits(&key[0], key.size());
        }
    };

    template<>
    class xxhash_function<tuple<uint64_t, uint64_t>>
    {
    public:
        size_t operator()(tuple<uint64_t, uint64_t> t) const
        {
            return XXH3_64bits(&t, tuple_sum_size(t));
        }
    };

    template<>
    class xxhash_function<tuple<int32_t, int32_t>>
    {
    public:
        size_t operator()(tuple<int32_t, int32_t> t) const
        {
            return XXH3_64bits(&t, tuple_sum_size(t));
        }
    };

    template<class Key>
    class custom_equalto;

    template<>
    class custom_equalto<string>
    {
    public:
        bool operator()(string const &lhs, string const &rhs) const
        {
            return lhs == rhs;
        }

        bool operator()(string const &lhs, string_view const &rhs) const
        {
            return lhs == rhs;
        }

        bool operator()(string_view const &lhs, string const &rhs) const
        {
            return lhs == rhs;
        }
    };

    template<>
    class custom_equalto<tuple<uint64_t, uint64_t>>
    {
    public:
        bool operator()(tuple<uint64_t, uint64_t> const &lhs, tuple<uint64_t, uint64_t> const &rhs) const
        {
            return lhs == rhs;
        }
    };

    template<>
    class custom_equalto<tuple<int32_t, int32_t>>
    {
    public:
        bool operator()(tuple<int32_t, int32_t> const &lhs, tuple<int32_t, int32_t> const &rhs) const
        {
            return lhs == rhs;
        }
    };

    template <typename Key, typename T>
    using lotr_flat_custom_map = robin_hood::unordered_flat_map<Key, T, xxhash_function<Key>, custom_equalto<Key>>;
    template <typename Key, typename T>
    using lotr_flat_map = robin_hood::unordered_flat_map<Key, T>;
}
