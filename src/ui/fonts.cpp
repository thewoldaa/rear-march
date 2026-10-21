// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/fonts.h"

#include <filesystem>
#include <string>
#include <vector>

#include <windows.h>

#include <imgui.h>

namespace rm {
namespace ui {
namespace {

constexpr float kBaseFontSize = 16.0f;

// Fonts are loaded from the system rather than bundled: Segoe UI ships with
// every supported Windows install, and bundling it would add megabytes to a
// small binary.
//
// Since ImGui 1.92 the atlas loads glyphs on demand, so no glyph ranges are
// specified here and no Build() call is needed.
constexpr const wchar_t* kLatinFonts[] = {
    L"C:\\Windows\\Fonts\\segoeui.ttf",
    L"C:\\Windows\\Fonts\\arial.ttf",
};

// Returns a stable, UTF-8 path to the first font that exists.
const std::string* first_existing(const wchar_t* const* candidates, size_t count) {
    static std::vector<std::string> storage;
    for (size_t i = 0; i < count; ++i) {
        const std::filesystem::path path(candidates[i]);
        std::error_code ec;
        if (std::filesystem::exists(path, ec)) {
            storage.push_back(path.string());
            return &storage.back();
        }
    }
    return nullptr;
}

}  // namespace

void load_fonts(float dpi_scale) {
    ImGuiIO& io = ImGui::GetIO();
    const float size = kBaseFontSize * dpi_scale;

    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 1;
    config.PixelSnapH = false;

    ImFont* base = nullptr;
    if (const std::string* latin = first_existing(kLatinFonts, std::size(kLatinFonts))) {
        base = io.Fonts->AddFontFromFileTTF(latin->c_str(), size, &config);
    }
    if (!base) {
        // The embedded default font is small but always available, so the UI
        // still renders if the system fonts are missing.
        ImFontConfig fallback;
        fallback.SizePixels = size;
        base = io.Fonts->AddFontDefault(&fallback);
    }
    io.FontDefault = base;
}

void rebuild_fonts(float dpi_scale) {
    // ClearFonts() drops the font data; the atlas rebuilds on the next frame
    // because the DX11 backend advertises ImGuiBackendFlags_RendererHasTextures.
    ImGui::GetIO().Fonts->ClearFonts();
    load_fonts(dpi_scale);
}

}  // namespace ui
}  // namespace rm
