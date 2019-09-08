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

#pragma once

#include <string>
#include <variant>
#include <array>
#include <vector>
#include <optional>
#include <lotr_flat_map.h>
#include <fov.h>

using namespace std;

namespace lotr {
    extern array<string, 38> const stats;

    struct stat_component {
        string name;
        int64_t value;

        stat_component(string name, int64_t value) : name(move(name)), value(value) {}
    };

    struct random_stat_component {
        string name;
        int64_t min;
        int64_t max;

        random_stat_component(string name, int64_t min, int64_t max) : name(move(name)), min(min), max(max) {}
    };

    struct skill_component {
        string name;
        int64_t value;
    };

    struct item_effect_component {
        string name;
        string tooltip;
        string message;
        uint32_t potency;
        uint32_t duration;
        uint32_t uses;
        uint32_t range;
        uint32_t chance;
        bool autocast;
        bool can_apply;
        vector<stat_component> stats;
    };

    struct item_required_skill_component {
        string name;
        uint32_t level;
    };

    struct global_item_component {
        string name;
        string desc;
        string type;
        //string required_quest;
        //string required_alignment;
        uint64_t quality;
        uint64_t enchant_level;
        uint64_t value;
        uint64_t sprite;
        uint64_t required_level;
        uint64_t tier;
        bool binds;
        bool tells_bind;
        optional<item_effect_component> effect;
        //optional<random_stat_component> random_trait_levels;
        //vector<string> random_trait_names;
        vector<string> required_professions;
        //vector<item_required_skill_component> required_skills;
        vector<stat_component> stats;
        vector<random_stat_component> random_stats;
    };

    struct item_component {
        string name;
        string desc;
        uint64_t value;
        uint64_t sprite;
        vector<stat_component> stats;
    };

    /*struct location_component {
        string map_name;
        uint32_t x;
        uint32_t y;
    };*/

    struct spawner_npc_id {
        uint32_t chance;
        string name;

        spawner_npc_id(uint32_t chance, string name) : chance(chance), name(move(name)) {}
    };

    struct spawner_script {
        uint32_t id;
        uint32_t respawn_rate;
        uint32_t initial_spawn;
        uint32_t max_creatures;
        uint32_t spawn_radius;
        uint32_t random_walk_radius;
        uint32_t leash_radius;
        uint32_t elite_tick_cap;
        uint32_t max_spawn;

        bool should_be_active;
        bool can_slow_down;
        bool should_serialize;
        bool always_spawn;
        bool require_dead_to_respawn;
        bool do_initial_spawn_immediately;

        vector<spawner_npc_id> npc_ids;
        vector<string> paths;
        vector<string> npc_ai_settings;
    };

    struct silver_purchases_component {
        string name;
        uint32_t count;
    };

    struct character_component {
        uint64_t id;
        string name;
        string allegiance;
        string alignment;
        string sex;
        string dir;
        string hostility;
        string character_class;
        string monster_class;
        string spawn_message;
        string sfx;

        uint32_t level;
        uint32_t highest_level;
        uint32_t sprite;
        uint32_t skill_on_kill;
        uint32_t gold;
        uint32_t give_xp;
        uint32_t sfx_max_chance;

        uint32_t x;
        uint32_t y;

        lotr_flat_custom_map<string, uint64_t> stats;
        lotr_flat_custom_map<string, item_component> items;
        vector<skill_component> skills;
        //location_component location;

        character_component() : id(), name(), allegiance(), alignment(), sex(), dir(), hostility(), character_class(), monster_class(), spawn_message(),
                          sfx(), level(), highest_level(), sprite(), skill_on_kill(), sfx_max_chance(), x(), y(), stats(), items(), skills() {}
    };

    struct pc_component : character_component {
        vector<silver_purchases_component> silver_purchases;
        bitset<power(fov_diameter)> fov;

        pc_component() : character_component(), silver_purchases(), fov() {}
    };

    struct npc_component : character_component {
        string npc_id;

        spawner_script *spawner;
        npc_component *agro_target;

        npc_component() : character_component(), npc_id(), spawner(nullptr), agro_target(nullptr) {}
    };

    struct global_npc_component {
        string name;
        string npc_id;
        string allegiance;
        string alignment;
        string sex;
        string dir;
        string hostility;
        string character_class;
        string monster_class;
        string spawn_message;
        string sfx;

        uint32_t level;
        uint32_t highest_level;
        vector<uint32_t> sprite;
        uint32_t skill_on_kill;
        uint32_t sfx_max_chance;

        vector<stat_component> stats;
        vector<random_stat_component> random_stats;
        vector<item_component> items;
        vector<skill_component> skills;
        //location_component location;
    };

    struct user_component {
        string name;
        string email;
        uint16_t subscription_tier;
        bool is_tester;
        bool is_game_master;
        uint16_t trial_ends;
        bool has_done_trial;
        string discord_tag;
        bool discord_online;
        vector<character_component> characters;
    };

    struct map_property {
        string name;
        variant<double, string> value;

        map_property(string name, double value) : name(move(name)), value(value) {}
        map_property(string name, string value) : name(move(name)), value(value) {}
    };

    struct map_object {
        uint32_t gid;
        uint32_t id;
        uint32_t x;
        uint32_t y;
        uint32_t width;
        uint32_t height;
        string name;
        string type;
        vector<map_property> properties;
        optional<spawner_script> script;

        map_object() : gid(0), id(0), x(0), y(0), width(0), height(0), name(), type(), properties(), script() {}
        map_object(uint32_t gid, uint32_t id, uint32_t x, uint32_t y, uint32_t width, uint32_t height, string name, string type, vector<map_property> properties, optional<spawner_script> script)
            : gid(gid), id(id), x(x), y(y), width(width), height(height), name(move(name)), type(move(type)), properties(move(properties)), script(move(script)) {}
    };

    struct map_layer {
        uint32_t x;
        uint32_t y;
        uint32_t width;
        uint32_t height;
        string name;
        string type;

        vector<map_object> objects;
        vector<uint32_t> data;

        map_layer(uint32_t x, uint32_t y, uint32_t width, uint32_t height, string name, string type, vector<map_object> objects, vector<uint32_t> data)
            : x(x), y(y), width(width), height(height), name(move(name)), type(move(type)), objects(move(objects)), data(move(data)) {}
    };

    struct map_component {
        uint32_t width;
        uint32_t height;
        string name;
        vector<map_property> properties;
        vector<map_layer> layers;
        vector<npc_component> npcs;
        vector<pc_component> players;

        map_component(uint32_t width, uint32_t height, string name, vector<map_property> properties, vector<map_layer> layers)
            : width(width), height(height), name(move(name)), properties(move(properties)), layers(move(layers)), npcs(), players() {}
    };

    // constants

    extern string const spawners_layer_name;
    extern string const tile_layer_name;
    extern string const object_layer_name;

}
