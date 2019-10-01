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

#include "load_npc.h"
#include <yaml-cpp/yaml.h>
#include <working_directory_manipulation.h>
#include "spdlog/spdlog.h"

using namespace std;
using namespace lotr;

#define NPC_STRING_FIELD(x) if(npc_node[ #x ]) { npc. x = npc_node[ #x ].as<string>(); }
#define NPC_UINT_FIELD(x) if(npc_node[ #x ]) { npc. x = npc_node[ #x ].as<uint32_t>(); }
#define NPC_BOOL_FIELD(x) if(npc_node[ #x ]) { npc. x = npc_node[ #x ].as<bool>(); }

vector<global_npc_component> lotr::load_global_npcs_from_file(string const &file) {
    spdlog::debug("[{}] loading npcs from file {}", __FUNCTION__, file);

    auto env_contents = read_whole_file(file);

    if(!env_contents) {
        return {};
    }

    YAML::Node tree = YAML::Load(env_contents.value());

    vector<global_npc_component> npcs;
    for(auto const &npc_node : tree) {
        global_npc_component npc{};
        npc.name = npc_node["name"].as<string>();

        spdlog::debug("[{}] loading npc {}", __FUNCTION__, npc.name);

        npc.npc_id = npc_node["npcId"].as<string>();

        NPC_STRING_FIELD(alignment)
        NPC_STRING_FIELD(hostility)

        if(npc_node["sprite"].Type() == YAML::NodeType::Sequence) {
            for(auto const &sprite_node : npc_node["sprite"]) {
                npc.sprite.emplace_back(sprite_node.as<uint32_t>());
            }
        } else {
            npc.sprite.emplace_back(npc_node["sprite"].as<uint32_t>());
        }

        npc.level = npc_node["level"].as<uint32_t>();

        if(npc_node["skillOnKill"]) {
            npc.skill_on_kill = npc_node["skillOnKill"].as<uint32_t>();
        }

        if(npc_node["spawnMessage"]) {
            npc.spawn_message = npc_node["spawnMessage"].as<string>();
        }

        if(npc_node["sfx"]) {
            npc.sfx = npc_node["sfx"].as<string>();
            npc.sfx_max_chance = npc_node["sfxMaxChance"].as<uint32_t>();
        }

        auto base_stat = npc_node["stats"].as<uint32_t >();
        for (auto const &stat : stat_names) {
            //spdlog::trace("[{}] loading stat {}", __FUNCTION__, stat);
            npc.stats.emplace_back(stat, base_stat);

            if (npc_node["otherStats"] && npc_node["otherStats"][stat]) {
                //spdlog::trace("[{}] loading stat {}:{}", __FUNCTION__, stat, npc_node["otherStats"][stat].as<int64_t>());
                npc.stats.back().value = npc_node["otherStats"][stat].as<int64_t>();
            }

            if (npc_node[stat]) {
                //spdlog::trace("[{}] loading random stat {}:{}-{}", __FUNCTION__, stat, npc_node[stat]["min"].as<int64_t>(), npc_node[stat]["max"].as<int64_t>());
                npc.random_stats.emplace_back(stat, npc_node[stat]["min"].as<int64_t>(), npc_node[stat]["max"].as<int64_t>());
            }
        }

        npc.random_stats.emplace_back("gold", npc_node["gold"]["min"].as<int64_t>(), npc_node["gold"]["max"].as<int64_t>());
        npc.random_stats.emplace_back("give_xp", npc_node["giveXp"]["min"].as<int64_t>(), npc_node["giveXp"]["max"].as<int64_t>());

        npcs.emplace_back(move(npc));
    }

    return npcs;
}

