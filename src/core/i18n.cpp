// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/i18n.h"

#include <array>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

namespace rm {
namespace {

using json = nlohmann::json;

struct LanguageInfo {
    Language language;
    std::string_view code;
    std::string_view native_name;
};

constexpr std::array<LanguageInfo, 2> kLanguages{{
    {Language::English, "en", "English"},
    {Language::Indonesian, "id", "Bahasa Indonesia"},
}};

const LanguageInfo* find_info(Language language) {
    for (const auto& info : kLanguages) {
        if (info.language == language) {
            return &info;
        }
    }
    return nullptr;
}

}  // namespace

std::string_view language_code(Language language) {
    const auto* info = find_info(language);
    return info ? info->code : "en";
}

Language language_from_code(std::string_view code) {
    for (const auto& info : kLanguages) {
        if (info.code == code) {
            return info.language;
        }
    }
    return Language::English;
}

std::string_view language_native_name(Language language) {
    const auto* info = find_info(language);
    return info ? info->native_name : "English";
}

void I18n::add(Language language, std::string_view key, std::string_view text) {
    auto& target = language == Language::English ? fallback_ : strings_;
    target[std::string(key)] = std::string(text);
}

bool I18n::load(Language language, const std::string& locale_dir) {
    language_ = language;

    // Start from empty tables. Keeping the previous language's entries would
    // mean a key that exists in the old locale but not in the new one shows the
    // old language, which looks like a broken translation rather than a missing
    // one. This invalidates every string_view handed out by tr(); reloading
    // must therefore happen between frames, never during one.
    strings_.clear();
    fallback_.clear();

    auto load_file = [&locale_dir](Language lang,
                                   std::unordered_map<std::string, std::string>& out) {
        const std::filesystem::path path =
            std::filesystem::path(locale_dir) / (std::string(language_code(lang)) + ".json");

        std::ifstream stream(path, std::ios::binary);
        if (!stream) {
            return false;
        }

        json root;
        try {
            root = json::parse(stream);
        } catch (const std::exception&) {
            return false;
        }
        if (!root.is_object()) {
            return false;
        }

        for (const auto& [key, value] : root.items()) {
            if (value.is_string()) {
                out[key] = value.get<std::string>();
            }
        }
        return true;
    };

    // English is the fallback layer, always attempted.
    load_file(Language::English, fallback_);

    if (language == Language::English) {
        return true;
    }

    return load_file(language, strings_);
}

std::string_view I18n::tr(std::string_view key) const {
    if (const auto it = strings_.find(std::string(key)); it != strings_.end()) {
        return it->second;
    }
    if (const auto it = fallback_.find(std::string(key)); it != fallback_.end()) {
        return it->second;
    }
    return key;
}

size_t I18n::key_count(Language language) const {
    return language == Language::English ? fallback_.size() : strings_.size();
}

}  // namespace rm
