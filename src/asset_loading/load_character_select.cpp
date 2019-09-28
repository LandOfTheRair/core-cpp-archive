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

#include "load_character_select.h"
#include <working_directory_manipulation.h>
#include "spdlog/spdlog.h"
#include <yaml-cpp/yaml.h>
#include <ecs/components.h>

using namespace std;
using namespace lotr;

void emplace_maxhp_maxmp(vector<stat_component> &stat_mods) {
    auto hp_it = find_if(begin(stat_mods), end(stat_mods), [](stat_component const &sc){ return sc.name == stat_hp;});
    if(hp_it != end(stat_mods)) {
        stat_mods.emplace_back(stat_max_hp, hp_it->value);
    }

    auto mp_it = find_if(begin(stat_mods), end(stat_mods), [](stat_component const &sc){ return sc.name == stat_mp;});
    if(mp_it != end(stat_mods)) {
        stat_mods.emplace_back(stat_max_mp, mp_it->value);
    }
}

optional<character_select_response> lotr::load_character_select() {
    auto env_contents = read_whole_file("assets/core/charselect.yml");

    if(!env_contents) {
        spdlog::trace("[{}] couldn't load character select!", __FUNCTION__);
        return {};
    }

    YAML::Node tree = YAML::Load(env_contents.value());

    if(!tree["baseStats"] || !tree["allegiances"] || !tree["classes"]) {
        spdlog::trace("[{}] couldn't load character select!", __FUNCTION__);
        return {};
    }

    vector<stat_component> base_stats;
    for (auto const &stat : stat_names) {
        if (tree["baseStats"][stat]) {
            base_stats.emplace_back(stat, tree["baseStats"][stat].as<int32_t>());
        }
    }

    emplace_maxhp_maxmp(base_stats);

    if(tree["baseStats"]["gold"]) {
        base_stats.emplace_back("gold", tree["baseStats"]["gold"].as<int32_t>());
    }

    vector<character_allegiance> allegiances;
    for(auto const &allegiance_node : tree["allegiances"]) {
        spdlog::trace("[{}] loading allegiance {}", __FUNCTION__, allegiance_node["name"].as<string>());

        vector<stat_component> stat_mods;
        vector<item_object> items;
        vector<skill_object> skills;
        if(allegiance_node["statMods"]) {
            for (auto const &stat : stat_names) {
                if (allegiance_node["statMods"][stat]) {
                    stat_mods.emplace_back(stat, allegiance_node["statMods"][stat].as<int32_t>());
                }
            }

            if(allegiance_node["statMods"]["gold"]) {
                stat_mods.emplace_back("gold", allegiance_node["statMods"]["gold"].as<int32_t>());
            }
        }

        if(allegiance_node["baseItems"]) {
            for (auto const &slot : slot_names) {
                if (allegiance_node["baseItems"][slot]) {
                    items.emplace_back(0, 0, 0, slot, "", allegiance_node["baseItems"][slot].as<string>(), vector<stat_component>{});
                }
            }
        }

        if(allegiance_node["baseSkills"]) {

        }

        emplace_maxhp_maxmp(stat_mods);

        allegiances.emplace_back(allegiance_node["name"].as<string>(), allegiance_node["description"].as<string>(), move(stat_mods), move(items), move(skills));
    }

    vector<character_class> classes;
    for(auto const &class_node : tree["classes"]) {
        spdlog::trace("[{}] loading class {}", __FUNCTION__, class_node["name"].as<string>());
        vector<stat_component> stat_mods;

        for (auto const &stat : stat_names) {
            if (class_node["statMods"][stat]) {
                stat_mods.emplace_back(stat, class_node["statMods"][stat].as<int32_t>());
            }
        }

        if(class_node["statMods"]["gold"]) {
            stat_mods.emplace_back("gold", class_node["statMods"]["gold"].as<int32_t>());
        }

        emplace_maxhp_maxmp(stat_mods);

        classes.emplace_back(class_node["name"].as<string>(), class_node["description"].as<string>(), stat_mods);
    }

    return character_select_response(base_stats, allegiances, classes);
}
