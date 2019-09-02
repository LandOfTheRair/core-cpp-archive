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

using namespace std;

namespace lotr {
    array<string, 39> const stats = {"str"s, "dex"s, "agi"s, "int"s, "wis"s, "wil"s, "luk"s, "cha"s, "con"s, "move"s, "hpregen"s, "mpregen"s, "hp"s, "mp"s,
                                     "weaponDamageRolls"s, "weaponArmorClass"s, "armorClass"s, "accuracy"s, "offense"s, "defense"s, "stealth"s,
                                     "perception"s, "physicalDamageBoost"s, "magicalDamageBoost"s, "healingBoost"s, "physicalDamageReflect"s,
                                     "magicalDamageReflect"s, "mitigation"s, "magicalResist"s, "physicalResist"s, "necroticResist"s, "energyResist"s,
                                     "waterResist"s, "fireResist"s, "iceResist"s, "poisonResist"s, "diseaseResist"s, "actionSpeed"s, "damageFactor"s};

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

    struct location_component {
        string map_name;
        uint32_t x;
        uint32_t y;
    };

    struct silver_purchases_component {
        string name;
        uint32_t count;
    };

    struct character_component {
        uint64_t database_id;
        string name;
        vector<stat_component> stats;
        vector<item_component> items;
        vector<skill_component> skills;
        vector<silver_purchases_component> silver_purchases;
        location_component location;
    };

    struct user_component {
        uint64_t database_id;
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

    // scripts

    struct spawner_npc_id {
        uint32_t chance;
        string name;

        spawner_npc_id(uint32_t chance, string name) : chance(chance), name(move(name)) {}
    };

    struct spawner_script {
        uint32_t respawn_rate;
        uint32_t initial_spawn;
        uint32_t max_creatures;
        uint32_t spawn_radius;
        uint32_t random_walk_radius;
        uint32_t leash_radius;

        vector<spawner_npc_id> npc_ids;
        vector<string> paths;
    };

    // maps

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

        map_component(uint32_t width, uint32_t height, string name, vector<map_property> properties, vector<map_layer> layers)
            : width(width), height(height), name(move(name)), properties(move(properties)), layers(move(layers)) {}
    };
}