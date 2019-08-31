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

#include "load_map.h"

#include <filesystem>
#include <rapidjson/document.h>
#include <working_directory_manipulation.h>
#include "spdlog/spdlog.h"

using namespace std;
using namespace rapidjson;
using namespace lotr;

optional<map_component> lotr::load_map_from_file(const string &file) {
    spdlog::debug("{} Loading load_map {}", __FUNCTION__, file);
    auto env_contents = read_whole_file(file);

    if(!env_contents) {
        return {};
    }

    Document d;
    d.Parse(env_contents->c_str(), env_contents->size());

    if (d.HasParseError() || !d.IsObject()) {
        spdlog::error("{} deserialize config.json failed", __FUNCTION__);
        return {};
    }

    uint32_t width = d["width"].GetUint();
    uint32_t height = d["height"].GetUint();
    string map_name = filesystem::path(file).filename();

    vector<map_layer> map_layers;
    auto& json_layers = d["layers"];

    for(SizeType i = 0; i < json_layers.Size(); i++) {
        spdlog::trace("{} layer {}", __FUNCTION__, i);
        auto& current_layer = json_layers[i];

        if (!current_layer.IsObject()) {
            spdlog::warn("{} deserialize failed", __FUNCTION__);
            return nullopt;
        }

        if(current_layer["type"].GetString() == "tilelayer"s) {
            vector<uint32_t> data;
            for (SizeType j = 0; j < current_layer["data"].Size(); j++) {
                data.push_back(current_layer["data"][j].GetUint());
            }
            map_layers.emplace_back(current_layer["x"].GetInt(), current_layer["y"].GetInt(), current_layer["width"].GetInt(),
                                    current_layer["height"].GetInt(), current_layer["type"].GetString(), vector<map_object>{}, data);
        }

        if(current_layer["type"].GetString() == "objectgroup"s) {
            vector<map_object> objects;
            for (SizeType j = 0; j < current_layer["objects"].Size(); j++) {

                auto& current_object = current_layer["objects"][j];
                // object properties
                auto& json_object_properties = current_object["properties"];
                vector<map_property> object_properties;
                for(auto it = json_object_properties.MemberBegin(); it != json_object_properties.MemberEnd(); it++) {
                    string name = it->name.GetString();
                    if(it->value.GetType() == Type::kStringType) {
                        object_properties.emplace_back(name, string(it->value.GetString()));
                    }

                    if(it->value.GetType() == Type::kNumberType) {
                        object_properties.emplace_back(name, it->value.GetUint());
                    }
                }

                // object itself
                objects.emplace_back(current_object["gid"].GetUint(), current_object["id"].GetUint(),
                        current_object["x"].GetUint(), current_object["y"].GetUint(), current_object["width"].GetUint(),
                        current_object["height"].GetUint(), current_object["name"].GetString(), current_object["type"].GetString(), object_properties);
            }
            map_layers.emplace_back(current_layer["x"].GetInt(), current_layer["y"].GetInt(), 0, 0, current_layer["type"].GetString(),
                                    objects, vector<uint32_t>{});
        }
    }

    vector<map_property> map_properties;
    auto& json_properties = d["properties"];

    for(auto it = json_properties.MemberBegin(); it != json_properties.MemberEnd(); it++) {
        string name = it->name.GetString();
        if(it->value.GetType() == Type::kStringType) {
            map_properties.emplace_back(name, string(it->value.GetString()));
        }

        if(it->value.GetType() == Type::kNumberType) {
            map_properties.emplace_back(name, it->value.GetUint());
        }
    }

    spdlog::trace("{} map {} {} {}", __FUNCTION__, width, height, map_name);
    return make_optional<map_component>(width, height, map_name, map_properties, map_layers);
}

