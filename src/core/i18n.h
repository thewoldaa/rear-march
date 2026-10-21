// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace rm {

// Supported UI languages.
enum class Language { English, Indonesian };

std::string_view language_code(Language language);
Language language_from_code(std::string_view code);
std::string_view language_native_name(Language language);

// Minimal translation table.
//
// Strings are looked up by flat dotted keys ("tab.turn.title"). A missing key
// falls back to English, then to the key itself, so a partially translated
// locale degrades gracefully instead of showing blanks.
class I18n {
public:
    // Loads the given locale. English is always loaded as the fallback.
    // `locale_dir` is searched for `<code>.json`.
    //
    // Reloading replaces both tables, which invalidates any string_view this
    // object has already handed out. Callers must reload between frames rather
    // than in the middle of drawing one.
    bool load(Language language, const std::string& locale_dir);

    // Inserts a translation directly. Used by tests.
    void add(Language language, std::string_view key, std::string_view text);

    Language language() const { return language_; }

    // Translated string for `key`, falling back to English then to `key`.
    std::string_view tr(std::string_view key) const;

    // Number of keys loaded for a language.
    size_t key_count(Language language) const;

private:
    Language language_ = Language::English;
    std::unordered_map<std::string, std::string> strings_;
    std::unordered_map<std::string, std::string> fallback_;
};

}  // namespace rm
