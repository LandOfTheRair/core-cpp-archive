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

#include <spdlog/spdlog.h>
#include <rapidjson/document.h>

#include "censor_sensor.h"
#include "working_directory_manipulation.h"

lotr::censor_sensor::censor_sensor(string const &profanity_dictionary_path) : _word_tiers() {
    auto dict_contents = read_whole_file(profanity_dictionary_path);
    if(!dict_contents) {
        throw runtime_error("no dict_contents");
    }

    rapidjson::Document d;
    d.Parse(dict_contents->c_str(), dict_contents->size());

    if (d.HasParseError() || !d.IsObject()) {
        spdlog::warn("{} deserialize failed", __FUNCTION__);
        throw runtime_error("deserialize failed");
    }

    for (auto iter = d.MemberBegin(); iter != d.MemberEnd(); ++iter){
        string word = iter->name.GetString();
        int tier = iter->value.GetInt();
        _word_tiers[word] = tier;
        _enabled_tiers.insert(tier);

        spdlog::trace("{} added profanity {}:{}", __FUNCTION__, word, tier);
    }
}

bool lotr::censor_sensor::is_profane(string const &phrase) {
    auto word_iter = _word_tiers.find(phrase);

    if(word_iter == _word_tiers.end()) {
        spdlog::trace("{} phrase {} not in dictionary", __FUNCTION__, phrase);
        return false;
    }

    auto tier_iter = _enabled_tiers.find(word_iter->second);

    if(tier_iter == _enabled_tiers.end()) {
        spdlog::trace("{} phrase {} tier {} not enabled", __FUNCTION__, phrase, word_iter->second);
        return false;
    }

    spdlog::trace("{} phrase {} profane", __FUNCTION__, phrase);
    return true;
}

void lotr::censor_sensor::enable_tier(uint32_t tier) {
    if(tier <= static_cast<uint32_t>(profanity_type::USER_ADDED)) {
        _enabled_tiers.insert(tier);
        spdlog::trace("{} tier {} enabled", __FUNCTION__, tier);
    }
}

void lotr::censor_sensor::disable_tier(uint32_t tier) {
    if(tier <= static_cast<uint32_t>(profanity_type::USER_ADDED)) {
        _enabled_tiers.erase(tier);
        spdlog::trace("{} tier {} disabled", __FUNCTION__, tier);
    }
}
