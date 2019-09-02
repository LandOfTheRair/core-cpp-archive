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

#include <catch2/catch.hpp>
#include <iostream>
#include <fstream>

#include "asset_loading/load_map.h"
#include "asset_loading/load_item.h"

using namespace std;
using namespace lotr;

TEST_CASE("item_loading tests") {
    ofstream item_file("test_item.json", ios_base::trunc);
    item_file << R"(
- name: Heniz Battlemage Gloves
  randomStats:
    offense:
      min: 1
      max: 2
    defense:
      min: 2
      max: 4
    hp:
      min: 10
      max: 30
    mp:
      min: 10
      max: 30
  tier: 2
  effect:
    name: MagicMissile
    potency: 10
    chance: 5
  desc: "a pair of dark gauntlets that emanate magic"
  sprite: 461
  binds: true

  requirements:
    level: 7
    profession:
      - "Mage"
      - "Thief"
)";

    item_file.close();

    auto item = load_global_items_from_file("test_item.json");

    REQUIRE(item.size() == 1);
    REQUIRE(item[0].required_level == 7);
    REQUIRE(item[0].tier == 2);
    REQUIRE(item[0].sprite == 461);
    REQUIRE(item[0].desc == "a pair of dark gauntlets that emanate magic"s);

    REQUIRE(item[0].random_stats.size() == 4);
    // order follows the global stats variable
    REQUIRE(item[0].random_stats[0].name == "hp");
    REQUIRE(item[0].random_stats[0].min == 10);
    REQUIRE(item[0].random_stats[0].max == 30);
    REQUIRE(item[0].random_stats[1].name == "mp");
    REQUIRE(item[0].random_stats[1].min == 10);
    REQUIRE(item[0].random_stats[1].max == 30);
    REQUIRE(item[0].random_stats[2].name == "offense");
    REQUIRE(item[0].random_stats[2].min == 1);
    REQUIRE(item[0].random_stats[2].max == 2);
    REQUIRE(item[0].random_stats[3].name == "defense");
    REQUIRE(item[0].random_stats[3].min == 2);
    REQUIRE(item[0].random_stats[3].max == 4);

    REQUIRE(item[0].required_professions.size() == 2);
    REQUIRE(item[0].required_professions[0] == "Mage"s);
    REQUIRE(item[0].required_professions[1] == "Thief"s);

    REQUIRE(item[0].effect);
    REQUIRE(item[0].effect->name == "MagicMissile"s);
    REQUIRE(item[0].effect->potency == 10);
    REQUIRE(item[0].effect->chance == 5);
}
