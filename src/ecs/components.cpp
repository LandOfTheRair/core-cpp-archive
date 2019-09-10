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
    array<string const, 38> const stats = {"str"s, "dex"s, "agi"s, "int"s, "wis"s, "wil"s, "luk"s, "cha"s, "con"s, "move"s, "hpregen"s, "mpregen"s, "hp"s, "mp"s,
                                     "weaponDamageRolls"s, "weaponArmorClass"s, "armorClass"s, "accuracy"s, "offense"s, "defense"s, "stealth"s,
                                     "perception"s, "physicalDamageBoost"s, "magicalDamageBoost"s, "healingBoost"s, "physicalDamageReflect"s,
                                     "magicalDamageReflect"s, "mitigation"s, "magicalResist"s, "physicalResist"s, "necroticResist"s, "energyResist"s,
                                     "waterResist"s, "fireResist"s, "iceResist"s, "poisonResist"s, "diseaseResist"s,
                                     "actionSpeed"s/*, "damageFactor"s TODO damage factor is a double :< */};


    string const spawners_layer_name = "Spawners";
    string const npcs_layer_name = "NPCs";
    string const tile_layer_name = "tilelayer";
    string const object_layer_name = "objectgroup";
    string const wall_layer_name = "Walls"s;
    string const opaque_layer_name = "OpaqueDecor"s;
}
