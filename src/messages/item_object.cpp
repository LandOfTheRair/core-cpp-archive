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

#include "item_object.h"
#include <ecs/components.h>

void lotr::write_item_object(rapidjson::Writer<rapidjson::StringBuffer> &writer, item_object const &obj) {
    writer.String(KEY_STRING("tier"));
    writer.Uint(obj.tier);

    writer.String(KEY_STRING("value"));
    writer.Uint(obj.value);

    writer.String(KEY_STRING("sprite"));
    writer.Uint(obj.sprite);

    writer.String(KEY_STRING("name"));
    writer.String(obj.name.c_str(), obj.name.size());

    writer.String(KEY_STRING("description"));
    writer.String(obj.description.c_str(), obj.description.size());

    writer.String(KEY_STRING("type"));
    writer.String(obj.type.c_str(), obj.type.size());

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
}
