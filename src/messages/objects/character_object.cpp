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

#include "character_object.h"
#include <ecs/components.h>

lotr::character_object::character_object(string name, string gender, string allegiance, string baseclass, string map_name, uint32_t level, uint32_t gold, uint32_t x, uint32_t y, vector<stat_component> stats, vector<item_object> items, vector<skill_object> skills) noexcept :
name(move(name)), gender(move(gender)), allegiance(move(allegiance)), baseclass(move(baseclass)), map_name(move(map_name)), level(level), gold(gold), x(x), y(y), stats(move(stats)), items(move(items)), skills(move(skills)) {}

void lotr::write_character_object(rapidjson::Writer<rapidjson::StringBuffer> &writer, character_object const &obj) {
    writer.String(KEY_STRING("name"));
    writer.String(obj.name.c_str(), obj.name.size());

    writer.String(KEY_STRING("gender"));
    writer.String(obj.gender.c_str(), obj.gender.size());

    writer.String(KEY_STRING("allegiance"));
    writer.String(obj.allegiance.c_str(), obj.allegiance.size());

    writer.String(KEY_STRING("baseclass"));
    writer.String(obj.baseclass.c_str(), obj.baseclass.size());

    writer.String(KEY_STRING("map_name"));
    writer.String(obj.map_name.c_str(), obj.map_name.size());

    writer.String(KEY_STRING("level"));
    writer.Uint(obj.level);

    writer.String(KEY_STRING("gold"));
    writer.Uint(obj.gold);

    writer.String(KEY_STRING("x"));
    writer.Uint(obj.x);

    writer.String(KEY_STRING("y"));
    writer.Uint(obj.y);

    writer.String(KEY_STRING("stats"));
    writer.StartArray();
    for(auto const &stat : obj.stats) {
        writer.StartObject();
        writer.String(KEY_STRING("name"));
        writer.String(stat.name.c_str(), stat.name.size());

        writer.String(KEY_STRING("value"));
        writer.Int64(stat.value);
        writer.EndObject();
    }
    writer.EndArray();

    writer.String(KEY_STRING("items"));
    writer.StartArray();
    for(auto const &item : obj.items) {
        writer.StartObject();
        write_item_object(writer, item);
        writer.EndObject();
    }
    writer.EndArray();

    writer.String(KEY_STRING("skills"));
    writer.StartArray();
    for(auto const &skill : obj.skills) {
        writer.StartObject();
        write_skill_object(writer, skill);
        writer.EndObject();
    }
    writer.EndArray();
}

bool lotr::read_character_object_into_vector(rapidjson::Value const &value, vector<character_object> &objs) {
    if(!value.IsObject() || !value.HasMember("name") || !value.HasMember("gender") ||
       !value.HasMember("allegiance") || !value.HasMember("baseclass") ||
       !value.HasMember("map_name") || !value.HasMember("x") ||
       !value.HasMember("level") || !value.HasMember("gold") ||
       !value.HasMember("y") || !value.HasMember("stats") ||
       !value.HasMember("items") || !value.HasMember("skills")) {
        return false;
    }

    auto &stats_array = value["stats"];
    if(!stats_array.IsArray()) {
        return false;
    }

    auto &items_array = value["items"];
    if(!items_array.IsArray()) {
        return false;
    }

    auto &skills_array = value["skills"];
    if(!skills_array.IsArray()) {
        return false;
    }

    vector<stat_component> stats;
    for(rapidjson::SizeType i = 0; i < stats_array.Size(); i++) {
        if(!stats_array[i].IsObject() || !stats_array[i].HasMember("name") || !stats_array[i].HasMember("value")) {
            return false;
        }

        stats.emplace_back(stats_array[i]["name"].GetString(), stats_array[i]["value"].GetUint());
    }

    vector<item_object> items;
    for(rapidjson::SizeType i = 0; i < items_array.Size(); i++) {
        if(!read_item_object_into_vector(items_array[i], items)) {
            return false;
        }
    }

    vector<skill_object> skills;
    for(rapidjson::SizeType i = 0; i < skills_array.Size(); i++) {
        if(!read_skill_object_into_vector(skills_array[i], skills)) {
            return false;
        }
    }

    objs.emplace_back(value["name"].GetString(), value["gender"].GetString(), value["allegiance"].GetString(), value["baseclass"].GetString(), value["map_name"].GetString(),
                      value["level"].GetUint(), value["gold"].GetUint(), value["x"].GetUint(), value["y"].GetUint(), move(stats), move(items), move(skills));
    return true;
}
