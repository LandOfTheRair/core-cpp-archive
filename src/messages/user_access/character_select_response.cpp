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

#include "character_select_response.h"
#include <spdlog/spdlog.h>
#include <rapidjson/writer.h>
#include <ecs/components.h>

using namespace lotr;
using namespace rapidjson;

string const character_select_response::type = "Game:character_select_response";

character_allegiance::character_allegiance(string name, string description, vector<stat_component> stat_mods, vector<item_object> items, vector<skill_object> skills) noexcept :
name(move(name)), description(move(description)), stat_mods(move(stat_mods)), items(move(items)), skills(move(skills)) {

}

character_class::character_class(string name, string description, vector<stat_component> stat_mods) noexcept :
name(move(name)), description(move(description)), stat_mods(move(stat_mods)) {

}

character_select_response::character_select_response(vector<stat_component> base_stats, vector<character_allegiance> allegiances, vector<character_class> classes) noexcept
        : base_stats(move(base_stats)), allegiances(move(allegiances)), classes(move(classes)) {

}

string character_select_response::serialize() const {
    StringBuffer sb;
    Writer<StringBuffer> writer(sb);

    writer.StartObject();

    writer.String(KEY_STRING("type"));
    writer.String(type.c_str(), type.size());

    writer.String(KEY_STRING("base_stats"));
    writer.StartArray();
    for(auto const &stat : base_stats) {
        writer.StartObject();
        writer.String(KEY_STRING("name"));
        writer.String(stat.name.c_str(), stat.name.size());

        writer.String(KEY_STRING("value"));
        writer.Int64(stat.value);
        writer.EndObject();
    }
    writer.EndArray();

    writer.String(KEY_STRING("allegiances"));
    writer.StartArray();
    for(auto const &allegiance : allegiances) {
        writer.StartObject();

        writer.String(KEY_STRING("name"));
        writer.String(allegiance.name.c_str(), allegiance.name.size());

        writer.String(KEY_STRING("description"));
        writer.String(allegiance.description.c_str(), allegiance.description.size());

        writer.String(KEY_STRING("stat_mods"));
        writer.StartArray();
        for(auto const &stat : allegiance.stat_mods) {
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
        for(auto const &item : allegiance.items) {
            writer.StartObject();

            write_item_object(writer, item);

            writer.EndObject();
        }
        writer.EndArray();

        writer.String(KEY_STRING("skills"));
        writer.StartArray();
        for(auto const &skill : allegiance.skills) {
            writer.StartObject();

            write_skill_object(writer, skill);

            writer.EndObject();
        }
        writer.EndArray();

        writer.EndObject();
    }
    writer.EndArray();

    writer.String(KEY_STRING("classes"));
    writer.StartArray();
    for(auto const &c : classes) {
        writer.StartObject();

        writer.String(KEY_STRING("name"));
        writer.String(c.name.c_str(), c.name.size());

        writer.String(KEY_STRING("description"));
        writer.String(c.description.c_str(), c.description.size());

        writer.String(KEY_STRING("stat_mods"));
        writer.StartArray();
        for(auto const &stat : c.stat_mods) {
            writer.StartObject();
            writer.String(KEY_STRING("name"));
            writer.String(stat.name.c_str(), stat.name.size());

            writer.String(KEY_STRING("value"));
            writer.Int64(stat.value);
            writer.EndObject();
        }
        writer.EndArray();

        writer.EndObject();
    }
    writer.EndArray();

    writer.EndObject();
    return sb.GetString();
}

optional<character_select_response> character_select_response::deserialize(rapidjson::Document const &d) {
    if (!d.HasMember("type") || !d.HasMember("base_stats")  || !d.HasMember("allegiances") || ! d.HasMember("classes")) {
        spdlog::warn("[character_select_response] deserialize failed");
        return nullopt;
    }

    if(d["type"].GetString() != type) {
        spdlog::warn("[character_select_response] deserialize failed wrong type");
        return nullopt;
    }

    vector<stat_component> base_stats;
    {
        auto &stats_array = d["base_stats"];
        if (!stats_array.IsArray()) {
            spdlog::warn("[character_select_response] deserialize failed1");
            return nullopt;
        }

        for (SizeType i = 0; i < stats_array.Size(); i++) {
            if (!stats_array[i].IsObject() ||
                !stats_array[i].HasMember("name") ||
                !stats_array[i].HasMember("value")) {
                spdlog::warn("[character_select_response] deserialize failed2");
                return nullopt;
            }
            base_stats.emplace_back(stats_array[i]["name"].GetString(), stats_array[i]["value"].GetInt64());
        }
    }


    vector<character_allegiance> allegiances;
    {
        auto &allegiance_array = d["allegiances"];
        if (!allegiance_array.IsArray()) {
            spdlog::warn("[character_select_response] deserialize failed3");
            return nullopt;
        }

        for (SizeType i = 0; i < allegiance_array.Size(); i++) {
            if (!allegiance_array[i].IsObject() ||
                !allegiance_array[i].HasMember("name") ||
                !allegiance_array[i].HasMember("description") ||
                !allegiance_array[i].HasMember("stat_mods") ||
                !allegiance_array[i].HasMember("items") ||
                !allegiance_array[i].HasMember("skills")) {
                spdlog::warn("[character_select_response] deserialize failed4");
                return nullopt;
            }

            vector<stat_component> allegiance_base_stats;
            {
                auto &allegiance_stat_mods_array = allegiance_array[i]["stat_mods"];
                if (!allegiance_stat_mods_array.IsArray()) {
                    spdlog::warn("[character_select_response] deserialize failed5");
                    return nullopt;
                }

                for (SizeType i2 = 0; i2 < allegiance_stat_mods_array.Size(); i2++) {
                    if (!allegiance_stat_mods_array[i2].IsObject() ||
                        !allegiance_stat_mods_array[i2].HasMember("name") ||
                        !allegiance_stat_mods_array[i2].HasMember("value")) {
                        spdlog::warn("[character_select_response] deserialize failed6");
                        return nullopt;
                    }
                    allegiance_base_stats.emplace_back(allegiance_stat_mods_array[i2]["name"].GetString(), allegiance_stat_mods_array[i2]["value"].GetInt64());
                }
            }

            vector<item_object> allegiance_items;
            {
                auto &allegiance_items_array = allegiance_array[i]["items"];
                if (!allegiance_items_array.IsArray()) {
                    spdlog::warn("[character_select_response] deserialize failed7");
                    return nullopt;
                }

                for (SizeType i2 = 0; i2 < allegiance_items_array.Size(); i2++) {
                    if (!allegiance_items_array[i2].IsObject() ||
                        !allegiance_items_array[i2].HasMember("name")) {
                        spdlog::warn("[character_select_response] deserialize failed8");
                        return nullopt;
                    }

                    allegiance_items.emplace_back(0, 0, 0, allegiance_items_array[i2]["name"].GetString(), "", "", vector<stat_component>{});
                }
            }

            vector<skill_object> allegiance_skills;
            {
                auto &allegiance_skills_array = allegiance_array[i]["skills"];
                if (!allegiance_skills_array.IsArray()) {
                    spdlog::warn("[character_select_response] deserialize failed9");
                    return nullopt;
                }

                for (SizeType i2 = 0; i2 < allegiance_skills_array.Size(); i2++) {
                    if (!allegiance_skills_array[i2].IsObject() ||
                        !allegiance_skills_array[i2].HasMember("name") ||
                        !allegiance_skills_array[i2].HasMember("value")) {
                        spdlog::warn("[character_select_response] deserialize failed10");
                        return nullopt;
                    }

                    allegiance_skills.emplace_back(allegiance_skills_array[i2]["name"].GetString(), allegiance_skills_array[i2]["value"].GetInt64());
                }
            }

            allegiances.emplace_back(allegiance_array[i]["name"].GetString(), allegiance_array[i]["description"].GetString(), move(allegiance_base_stats),
                                     move(allegiance_items), move(allegiance_skills));
        }
    }

    vector<character_class> classes;
    {
        auto &classes_array = d["classes"];
        if (!classes_array.IsArray()) {
            spdlog::warn("[character_select_response] deserialize failed11");
            return nullopt;
        }

        for (SizeType i = 0; i < classes_array.Size(); i++) {
            if (!classes_array[i].IsObject() ||
                !classes_array[i].HasMember("name") ||
                !classes_array[i].HasMember("description") ||
                !classes_array[i].HasMember("stat_mods")) {
                spdlog::warn("[character_select_response] deserialize failed12");
                return nullopt;
            }

            vector<stat_component> class_stat_mods;
            {
                auto &class_stat_mods_array = classes_array[i]["stat_mods"];
                if (!class_stat_mods_array.IsArray()) {
                    spdlog::warn("[character_select_response] deserialize failed13");
                    return nullopt;
                }

                for (SizeType i2 = 0; i2 < class_stat_mods_array.Size(); i2++) {
                    if (!class_stat_mods_array[i2].IsObject() ||
                        !class_stat_mods_array[i2].HasMember("name") ||
                        !class_stat_mods_array[i2].HasMember("value")) {
                        spdlog::warn("[character_select_response] deserialize failed14");
                        return nullopt;
                    }
                    class_stat_mods.emplace_back(class_stat_mods_array[i2]["name"].GetString(), class_stat_mods_array[i2]["value"].GetInt64());
                }
            }

            classes.emplace_back(classes_array[i]["name"].GetString(), classes_array[i]["description"].GetString(), move(class_stat_mods));
        }
    }

    return character_select_response(move(base_stats), move(allegiances), move(classes));
}
