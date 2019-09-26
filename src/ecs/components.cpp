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

#include "components.h"
namespace  lotr {
    string const north_direction = "N";
    string const east_direction = "E";
    string const south_direction = "S";
    string const west_direction = "W";
    string const north_east_direction = "NE";
    string const north_west_direction = "NW";
    string const south_east_direction = "SE";
    string const south_west_direction = "SW";

    string const stat_str = "str";
    string const stat_dex = "dex";
    string const stat_agi = "agi";
    string const stat_int = "int";
    string const stat_wis = "wis";
    string const stat_wil = "wil";
    string const stat_luk = "luk";
    string const stat_cha = "cha";
    string const stat_con = "con";
    string const stat_move = "move";
    string const stat_hpregen = "hpregen";
    string const stat_mpregen = "mpregen";
    string const stat_hp = "hp";
    string const stat_max_hp = "maxhp";
    string const stat_mp = "mp";
    string const stat_max_mp = "maxmp";
    string const stat_hweapon_damage_rolls = "weaponDamageRolls";
    string const stat_weapon_armor_class = "weaponArmorClass";
    string const stat_armor_class = "armorClass";
    string const stat_accuracy = "accuracy";
    string const stat_offense = "offense";
    string const stat_defense = "defense";
    string const stat_stealth = "stealth";
    string const stat_perception = "perception";
    string const stat_physical_damage_boost = "physicalDamageBoost";
    string const stat_magical_damage_boost = "magicalDamageBoost";
    string const stat_healing_boost = "healingBoost";
    string const stat_physical_damage_reflect = "physicalDamageReflect";
    string const stat_magical_damage_reflect = "magicalDamageReflect";
    string const stat_mitigation = "mitigation";
    string const stat_magical_resist = "magicalResist";
    string const stat_physical_resist = "physicalResist";
    string const stat_necrotic_resist = "necroticResist";
    string const stat_energy_resist = "energyResist";
    string const stat_water_resist = "waterResist";
    string const stat_fire_resist = "fireResist";
    string const stat_ice_resist = "iceResist";
    string const stat_poison_resist = "poisonResist";
    string const stat_disease_resist = "diseaseResist";
    string const stat_action_speed = "actionSpeed";
    /*, "damageFactor"s TODO damage factor is a double :< */

    string const hostility_never = "always";
    string const hostility_on_hit = "onHit";
    string const hostility_faction = "faction";
    string const hostility_always = "always";

    extern string const gear_slot_right_hand = "rightHand";
    extern string const gear_slot_left_hand = "leftHand";
    extern string const gear_slot_armor = "armor";
    extern string const gear_slot_robe1 = "robe1";
    extern string const gear_slot_robe2 = "robe2";
    extern string const gear_slot_ring1 = "ring1";
    extern string const gear_slot_ring2 = "ring2";
    extern string const gear_slot_head = "head";
    extern string const gear_slot_next = "next";
    extern string const gear_slot_waist = "waist";
    extern string const gear_slot_wrist = "wrist";
    extern string const gear_slot_hands = "hands";
    extern string const gear_slot_feet = "feet";
    extern string const gear_slot_ear = "ear";

    array<string const, 40> const stat_names = {stat_str, stat_dex, stat_agi, stat_int, stat_wis, stat_wil, stat_luk, stat_cha, stat_con, stat_move,
                                                stat_hpregen, stat_mpregen, stat_hp, stat_mp, stat_max_hp, stat_max_mp, stat_hweapon_damage_rolls, stat_weapon_armor_class, stat_armor_class,
                                                stat_accuracy, stat_offense, stat_defense, stat_stealth, stat_perception, stat_physical_damage_boost, stat_magical_damage_boost,
                                                stat_healing_boost, stat_physical_damage_reflect, stat_magical_damage_reflect, stat_mitigation, stat_magical_resist,
                                                stat_physical_resist, stat_necrotic_resist, stat_energy_resist, stat_water_resist, stat_fire_resist, stat_ice_resist,
                                                stat_poison_resist, stat_disease_resist, stat_action_speed };

    array<string const, 14> const slot_names = {gear_slot_right_hand, gear_slot_left_hand, gear_slot_armor, gear_slot_robe1, gear_slot_robe2, gear_slot_ring1,
                                                gear_slot_ring2, gear_slot_head, gear_slot_next, gear_slot_waist, gear_slot_wrist, gear_slot_hands,
                                                gear_slot_feet, gear_slot_ear};


    global_npc_component* get_global_npc_by_npc_id(entt::registry &registry, string const &npc_id) {
        auto gnpc_view = registry.view<global_npc_component>();

        for(auto gnpc_entity : gnpc_view) {
            global_npc_component &gnpc = gnpc_view.get(gnpc_entity);

            if(gnpc.npc_id != npc_id) {
                continue;
            }

            return &gnpc;
        }

        return nullptr;
    }

    map_component* get_map_by_name(entt::registry &registry, string const &name) {
        auto map_view = registry.view<map_component>();

        for(auto m_entity : map_view) {
            map_component &m = map_view.get(m_entity);

            if(m.name != name) {
                continue;
            }

            return &m;
        }

        return nullptr;
    }
}
