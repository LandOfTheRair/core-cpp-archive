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

#include "load_item.h"
#include <yaml-cpp/yaml.h>
#include <working_directory_manipulation.h>
#include "spdlog/spdlog.h"

using namespace std;
using namespace lotr;

#define EFFECT_STRING_FIELD(x) if(item_node["effect"][ #x ]) { effect. x = item_node["effect"][ #x ].as<string>(); }
#define EFFECT_UINT_FIELD(x) if(item_node["effect"][ #x ]) { effect. x = item_node["effect"][ #x ].as<uint32_t>(); }
#define EFFECT_BOOL_FIELD(x) if(item_node["effect"][ #x ]) { effect. x = item_node["effect"][ #x ].as<bool>(); }

#define ITEM_STRING_FIELD(x) if(item_node[ #x ]) { item. x = item_node[ #x ].as<string>(); }
#define ITEM_UINT_FIELD(x) if(item_node[ #x ]) { item. x = item_node[ #x ].as<uint32_t>(); }
#define ITEM_BOOL_FIELD(x) if(item_node[ #x ]) { item. x = item_node[ #x ].as<bool>(); }

vector<global_item_component> lotr::load_global_items_from_file(string const &file) {
    spdlog::debug("[{}] loading items from file {}", __FUNCTION__, file);

    auto env_contents = read_whole_file(file);

    if(!env_contents) {
        return {};
    }

    YAML::Node tree = YAML::Load(env_contents.value());

    vector<global_item_component> items;
    for(auto const &item_node : tree) {
        global_item_component item{};
        item.name = item_node["name"].as<string>();
        item.desc = item_node["desc"].as<string>();
        item.sprite = item_node["sprite"].as<uint64_t>();

        spdlog::debug("[{}] loading item {}", __FUNCTION__, item.name);

        for (auto const &stat : stats) {
            if (item_node["stats"] && item_node["stats"][stat]) {
                spdlog::trace("[{}] loading stat {}:{}", __FUNCTION__, stat, item_node["stats"][stat].as<int64_t>());
                item.stats.emplace_back(stat, item_node["stats"][stat].as<int64_t>());
            }

            if (item_node["randomStats"] && item_node["randomStats"][stat]) {
                spdlog::trace("[{}] loading random stat {}:{}-{}", __FUNCTION__, stat, item_node["randomStats"][stat]["min"].as<int64_t>(), item_node["randomStats"][stat]["max"].as<int64_t>());
                item.random_stats.emplace_back(stat, item_node["randomStats"][stat]["min"].as<int64_t>(), item_node["randomStats"][stat]["max"].as<int64_t>());
            }
        }

        ITEM_STRING_FIELD(type)

        ITEM_UINT_FIELD(quality)
        ITEM_UINT_FIELD(enchant_level)
        ITEM_UINT_FIELD(value)
        ITEM_UINT_FIELD(tier)

        ITEM_BOOL_FIELD(binds)
        ITEM_BOOL_FIELD(tells_bind)

        if (item_node["requirements"]) {
            if (item_node["requirements"]["level"]) {
                item.required_level = item_node["requirements"]["level"].as<uint64_t>();
            }

            if (item_node["requirements"]["profession"]) {
                for(auto const &profession_node : item_node["requirements"]["profession"]) {
                    spdlog::trace("[{}] loading requirements profession {}", __FUNCTION__, profession_node.as<string>());
                    item.required_professions.emplace_back(profession_node.as<string>());
                }
            }
        }

        if (item_node["effect"]) {
            item_effect_component effect{};

            EFFECT_STRING_FIELD(name)
            EFFECT_STRING_FIELD(tooltip)
            EFFECT_STRING_FIELD(message)

            EFFECT_UINT_FIELD(potency)
            EFFECT_UINT_FIELD(duration)
            EFFECT_UINT_FIELD(uses)
            EFFECT_UINT_FIELD(range)
            EFFECT_UINT_FIELD(chance)

            EFFECT_BOOL_FIELD(autocast)
            EFFECT_BOOL_FIELD(can_apply)

            for (auto const &stat : stats) {
                if (item_node["effect"]["stats"] && item_node["effect"]["stats"][stat]) {
                    spdlog::trace("[{}] loading effect stat {}:{}", __FUNCTION__, stat, item_node["effect"]["stats"][stat].as<int64_t>());
                    effect.stats.emplace_back(stat, item_node["effect"]["stats"][stat].as<int64_t>());
                }
            }

            item.effect = effect;
        }

        items.emplace_back(move(item));
    }

    return items;
}

