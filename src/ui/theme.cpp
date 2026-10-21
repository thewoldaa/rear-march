// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/theme.h"

namespace rm {
namespace ui {
namespace {

constexpr ImVec4 rgb(int r, int g, int b, float a = 1.0f) {
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a);
}

}  // namespace

void apply_theme(float dpi_scale) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark(&style);

    // A compact, flat look. Rounded corners are subtle: this is a utility
    // window, not a consumer app.
    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 3.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.WindowPadding = ImVec2(16.0f, 14.0f);
    style.FramePadding = ImVec2(10.0f, 5.0f);
    style.ItemSpacing = ImVec2(10.0f, 9.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.SeparatorTextBorderSize = 1.0f;
    style.SeparatorTextPadding = ImVec2(18.0f, 7.0f);

    // Scale metrics with the monitor DPI. Fonts are scaled separately when the
    // atlas is rebuilt.
    style.ScaleAllSizes(dpi_scale);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = rgb(0xE4, 0xE8, 0xEE);
    colors[ImGuiCol_TextDisabled] = rgb(0x77, 0x80, 0x8C);

    colors[ImGuiCol_WindowBg] = rgb(0x15, 0x18, 0x1D);
    colors[ImGuiCol_ChildBg] = rgb(0x1A, 0x1E, 0x24);
    colors[ImGuiCol_PopupBg] = rgb(0x1A, 0x1E, 0x24);
    colors[ImGuiCol_Border] = rgb(0x2A, 0x30, 0x39);
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

    colors[ImGuiCol_FrameBg] = rgb(0x22, 0x27, 0x2F);
    colors[ImGuiCol_FrameBgHovered] = rgb(0x2C, 0x33, 0x3D);
    colors[ImGuiCol_FrameBgActive] = rgb(0x34, 0x3C, 0x48);

    colors[ImGuiCol_TitleBg] = rgb(0x11, 0x14, 0x18);
    colors[ImGuiCol_TitleBgActive] = rgb(0x15, 0x18, 0x1D);
    colors[ImGuiCol_MenuBarBg] = rgb(0x11, 0x14, 0x18);

    colors[ImGuiCol_ScrollbarBg] = rgb(0x11, 0x14, 0x18);
    colors[ImGuiCol_ScrollbarGrab] = rgb(0x33, 0x3A, 0x45);
    colors[ImGuiCol_ScrollbarGrabHovered] = rgb(0x3E, 0x47, 0x54);
    colors[ImGuiCol_ScrollbarGrabActive] = rgb(0x4A, 0x55, 0x64);

    colors[ImGuiCol_CheckMark] = rgb(0x4C, 0xC2, 0xA6);
    colors[ImGuiCol_SliderGrab] = rgb(0x4C, 0xC2, 0xA6);
    colors[ImGuiCol_SliderGrabActive] = rgb(0x62, 0xD6, 0xB9);

    colors[ImGuiCol_Button] = rgb(0x26, 0x2C, 0x35);
    colors[ImGuiCol_ButtonHovered] = rgb(0x32, 0x3A, 0x46);
    colors[ImGuiCol_ButtonActive] = rgb(0x3C, 0x46, 0x54);

    colors[ImGuiCol_Header] = rgb(0x26, 0x2C, 0x35);
    colors[ImGuiCol_HeaderHovered] = rgb(0x32, 0x3A, 0x46);
    colors[ImGuiCol_HeaderActive] = rgb(0x3C, 0x46, 0x54);

    colors[ImGuiCol_Separator] = rgb(0x2A, 0x30, 0x39);
    colors[ImGuiCol_SeparatorHovered] = rgb(0x4C, 0xC2, 0xA6);
    colors[ImGuiCol_SeparatorActive] = rgb(0x62, 0xD6, 0xB9);

    colors[ImGuiCol_ResizeGrip] = rgb(0x26, 0x2C, 0x35);
    colors[ImGuiCol_ResizeGripHovered] = rgb(0x4C, 0xC2, 0xA6);
    colors[ImGuiCol_ResizeGripActive] = rgb(0x62, 0xD6, 0xB9);

    colors[ImGuiCol_TextSelectedBg] = rgb(0x4C, 0xC2, 0xA6, 0.28f);
    colors[ImGuiCol_DragDropTarget] = rgb(0x4C, 0xC2, 0xA6);
    colors[ImGuiCol_NavCursor] = rgb(0x4C, 0xC2, 0xA6);
    colors[ImGuiCol_NavWindowingHighlight] = rgb(0x4C, 0xC2, 0xA6, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = rgb(0x00, 0x00, 0x00, 0.40f);
    colors[ImGuiCol_ModalWindowDimBg] = rgb(0x00, 0x00, 0x00, 0.55f);
}

}  // namespace ui
}  // namespace rm
