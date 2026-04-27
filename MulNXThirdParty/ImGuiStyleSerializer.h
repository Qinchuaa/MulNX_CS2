// ImGuiStyleYamlSerialization.h
// Dear ImGui v1.92.7 -> YAML serialization (function-based, no template specialization)
#pragma once

#include <yaml-cpp/yaml.h>
#include <MulNXThirdParty/imgui_d11/imgui.h>
#include <unordered_map>
#include <string>

namespace ImGuiYaml {

    // ---------- ImVec2 ----------
    inline YAML::Node EncodeImVec2(const ImVec2& v) {
        YAML::Node node;
        node.push_back(v.x);
        node.push_back(v.y);
        return node;
    }

    inline bool DecodeImVec2(const YAML::Node& node, ImVec2& v) {
        if (!node.IsSequence() || node.size() != 2) return false;
        v.x = node[0].as<float>();
        v.y = node[1].as<float>();
        return true;
    }

    // ---------- ImVec4 ----------
    inline YAML::Node EncodeImVec4(const ImVec4& v) {
        YAML::Node node;
        node.push_back(v.x);
        node.push_back(v.y);
        node.push_back(v.z);
        node.push_back(v.w);
        return node;
    }

    inline bool DecodeImVec4(const YAML::Node& node, ImVec4& v) {
        if (!node.IsSequence() || node.size() != 4) return false;
        v.x = node[0].as<float>();
        v.y = node[1].as<float>();
        v.z = node[2].as<float>();
        v.w = node[3].as<float>();
        return true;
    }

    // ---------- ImGuiDir ----------
    inline YAML::Node EncodeImGuiDir(ImGuiDir dir) {
        static const std::unordered_map<ImGuiDir, std::string> map = {
            {ImGuiDir_None, "None"},
            {ImGuiDir_Left, "Left"},
            {ImGuiDir_Right, "Right"},
            {ImGuiDir_Up, "Up"},
            {ImGuiDir_Down, "Down"}
        };
        auto it = map.find(dir);
        return YAML::Node(it != map.end() ? it->second : "Unknown");
    }

    inline bool DecodeImGuiDir(const YAML::Node& node, ImGuiDir& dir) {
        if (!node.IsScalar()) return false;
        std::string str = node.Scalar();
        static const std::unordered_map<std::string, ImGuiDir> map = {
            {"None", ImGuiDir_None},
            {"Left", ImGuiDir_Left},
            {"Right", ImGuiDir_Right},
            {"Up", ImGuiDir_Up},
            {"Down", ImGuiDir_Down}
        };
        auto it = map.find(str);
        if (it != map.end()) {
            dir = it->second;
            return true;
        }
        return false;
    }

    // ---------- ImGuiCol ----------
    // typedef int, we map to string names
    inline YAML::Node EncodeImGuiCol(ImGuiCol col) {
        static const std::unordered_map<int, std::string> map = {
            {ImGuiCol_Text, "Text"},
            {ImGuiCol_TextDisabled, "TextDisabled"},
            {ImGuiCol_WindowBg, "WindowBg"},
            {ImGuiCol_ChildBg, "ChildBg"},
            {ImGuiCol_PopupBg, "PopupBg"},
            {ImGuiCol_Border, "Border"},
            {ImGuiCol_BorderShadow, "BorderShadow"},
            {ImGuiCol_FrameBg, "FrameBg"},
            {ImGuiCol_FrameBgHovered, "FrameBgHovered"},
            {ImGuiCol_FrameBgActive, "FrameBgActive"},
            {ImGuiCol_TitleBg, "TitleBg"},
            {ImGuiCol_TitleBgActive, "TitleBgActive"},
            {ImGuiCol_TitleBgCollapsed, "TitleBgCollapsed"},
            {ImGuiCol_MenuBarBg, "MenuBarBg"},
            {ImGuiCol_ScrollbarBg, "ScrollbarBg"},
            {ImGuiCol_ScrollbarGrab, "ScrollbarGrab"},
            {ImGuiCol_ScrollbarGrabHovered, "ScrollbarGrabHovered"},
            {ImGuiCol_ScrollbarGrabActive, "ScrollbarGrabActive"},
            {ImGuiCol_CheckMark, "CheckMark"},
            {ImGuiCol_SliderGrab, "SliderGrab"},
            {ImGuiCol_SliderGrabActive, "SliderGrabActive"},
            {ImGuiCol_Button, "Button"},
            {ImGuiCol_ButtonHovered, "ButtonHovered"},
            {ImGuiCol_ButtonActive, "ButtonActive"},
            {ImGuiCol_Header, "Header"},
            {ImGuiCol_HeaderHovered, "HeaderHovered"},
            {ImGuiCol_HeaderActive, "HeaderActive"},
            {ImGuiCol_Separator, "Separator"},
            {ImGuiCol_SeparatorHovered, "SeparatorHovered"},
            {ImGuiCol_SeparatorActive, "SeparatorActive"},
            {ImGuiCol_ResizeGrip, "ResizeGrip"},
            {ImGuiCol_ResizeGripHovered, "ResizeGripHovered"},
            {ImGuiCol_ResizeGripActive, "ResizeGripActive"},
            {ImGuiCol_InputTextCursor, "InputTextCursor"},
            {ImGuiCol_TabHovered, "TabHovered"},
            {ImGuiCol_Tab, "Tab"},
            {ImGuiCol_TabSelected, "TabSelected"},
            {ImGuiCol_TabSelectedOverline, "TabSelectedOverline"},
            {ImGuiCol_TabDimmed, "TabDimmed"},
            {ImGuiCol_TabDimmedSelected, "TabDimmedSelected"},
            {ImGuiCol_TabDimmedSelectedOverline, "TabDimmedSelectedOverline"},
            {ImGuiCol_PlotLines, "PlotLines"},
            {ImGuiCol_PlotLinesHovered, "PlotLinesHovered"},
            {ImGuiCol_PlotHistogram, "PlotHistogram"},
            {ImGuiCol_PlotHistogramHovered, "PlotHistogramHovered"},
            {ImGuiCol_TableHeaderBg, "TableHeaderBg"},
            {ImGuiCol_TableBorderStrong, "TableBorderStrong"},
            {ImGuiCol_TableBorderLight, "TableBorderLight"},
            {ImGuiCol_TableRowBg, "TableRowBg"},
            {ImGuiCol_TableRowBgAlt, "TableRowBgAlt"},
            {ImGuiCol_TextLink, "TextLink"},
            {ImGuiCol_TextSelectedBg, "TextSelectedBg"},
            {ImGuiCol_TreeLines, "TreeLines"},
            {ImGuiCol_DragDropTarget, "DragDropTarget"},
            {ImGuiCol_DragDropTargetBg, "DragDropTargetBg"},
            {ImGuiCol_UnsavedMarker, "UnsavedMarker"},
            {ImGuiCol_NavCursor, "NavCursor"},
            {ImGuiCol_NavWindowingHighlight, "NavWindowingHighlight"},
            {ImGuiCol_NavWindowingDimBg, "NavWindowingDimBg"},
            {ImGuiCol_ModalWindowDimBg, "ModalWindowDimBg"},
        };
        auto it = map.find(static_cast<int>(col));
        return YAML::Node(it != map.end() ? it->second : "Unknown");
    }

    inline bool DecodeImGuiCol(const YAML::Node& node, ImGuiCol& col) {
        if (!node.IsScalar()) return false;
        std::string str = node.Scalar();
        static const std::unordered_map<std::string, int> map = {
            {"Text", ImGuiCol_Text},
            {"TextDisabled", ImGuiCol_TextDisabled},
            {"WindowBg", ImGuiCol_WindowBg},
            {"ChildBg", ImGuiCol_ChildBg},
            {"PopupBg", ImGuiCol_PopupBg},
            {"Border", ImGuiCol_Border},
            {"BorderShadow", ImGuiCol_BorderShadow},
            {"FrameBg", ImGuiCol_FrameBg},
            {"FrameBgHovered", ImGuiCol_FrameBgHovered},
            {"FrameBgActive", ImGuiCol_FrameBgActive},
            {"TitleBg", ImGuiCol_TitleBg},
            {"TitleBgActive", ImGuiCol_TitleBgActive},
            {"TitleBgCollapsed", ImGuiCol_TitleBgCollapsed},
            {"MenuBarBg", ImGuiCol_MenuBarBg},
            {"ScrollbarBg", ImGuiCol_ScrollbarBg},
            {"ScrollbarGrab", ImGuiCol_ScrollbarGrab},
            {"ScrollbarGrabHovered", ImGuiCol_ScrollbarGrabHovered},
            {"ScrollbarGrabActive", ImGuiCol_ScrollbarGrabActive},
            {"CheckMark", ImGuiCol_CheckMark},
            {"SliderGrab", ImGuiCol_SliderGrab},
            {"SliderGrabActive", ImGuiCol_SliderGrabActive},
            {"Button", ImGuiCol_Button},
            {"ButtonHovered", ImGuiCol_ButtonHovered},
            {"ButtonActive", ImGuiCol_ButtonActive},
            {"Header", ImGuiCol_Header},
            {"HeaderHovered", ImGuiCol_HeaderHovered},
            {"HeaderActive", ImGuiCol_HeaderActive},
            {"Separator", ImGuiCol_Separator},
            {"SeparatorHovered", ImGuiCol_SeparatorHovered},
            {"SeparatorActive", ImGuiCol_SeparatorActive},
            {"ResizeGrip", ImGuiCol_ResizeGrip},
            {"ResizeGripHovered", ImGuiCol_ResizeGripHovered},
            {"ResizeGripActive", ImGuiCol_ResizeGripActive},
            {"InputTextCursor", ImGuiCol_InputTextCursor},
            {"TabHovered", ImGuiCol_TabHovered},
            {"Tab", ImGuiCol_Tab},
            {"TabSelected", ImGuiCol_TabSelected},
            {"TabSelectedOverline", ImGuiCol_TabSelectedOverline},
            {"TabDimmed", ImGuiCol_TabDimmed},
            {"TabDimmedSelected", ImGuiCol_TabDimmedSelected},
            {"TabDimmedSelectedOverline", ImGuiCol_TabDimmedSelectedOverline},
            {"PlotLines", ImGuiCol_PlotLines},
            {"PlotLinesHovered", ImGuiCol_PlotLinesHovered},
            {"PlotHistogram", ImGuiCol_PlotHistogram},
            {"PlotHistogramHovered", ImGuiCol_PlotHistogramHovered},
            {"TableHeaderBg", ImGuiCol_TableHeaderBg},
            {"TableBorderStrong", ImGuiCol_TableBorderStrong},
            {"TableBorderLight", ImGuiCol_TableBorderLight},
            {"TableRowBg", ImGuiCol_TableRowBg},
            {"TableRowBgAlt", ImGuiCol_TableRowBgAlt},
            {"TextLink", ImGuiCol_TextLink},
            {"TextSelectedBg", ImGuiCol_TextSelectedBg},
            {"TreeLines", ImGuiCol_TreeLines},
            {"DragDropTarget", ImGuiCol_DragDropTarget},
            {"DragDropTargetBg", ImGuiCol_DragDropTargetBg},
            {"UnsavedMarker", ImGuiCol_UnsavedMarker},
            {"NavCursor", ImGuiCol_NavCursor},
            {"NavWindowingHighlight", ImGuiCol_NavWindowingHighlight},
            {"NavWindowingDimBg", ImGuiCol_NavWindowingDimBg},
            {"ModalWindowDimBg", ImGuiCol_ModalWindowDimBg},
        };
        auto it = map.find(str);
        if (it != map.end()) {
            col = static_cast<ImGuiCol>(it->second);
            return true;
        }
        return false;
    }

    // ---------- ImGuiTreeNodeFlags (bitmask) ----------
    inline YAML::Node EncodeImGuiTreeNodeFlags(ImGuiTreeNodeFlags flags) {
        return YAML::Node(static_cast<int>(flags));
    }
    inline bool DecodeImGuiTreeNodeFlags(const YAML::Node& node, ImGuiTreeNodeFlags& flags) {
        if (!node.IsScalar()) return false;
        flags = static_cast<ImGuiTreeNodeFlags>(node.as<int>());
        return true;
    }

    // ---------- ImGuiHoveredFlags (bitmask) ----------
    inline YAML::Node EncodeImGuiHoveredFlags(ImGuiHoveredFlags flags) {
        return YAML::Node(static_cast<int>(flags));
    }
    inline bool DecodeImGuiHoveredFlags(const YAML::Node& node, ImGuiHoveredFlags& flags) {
        if (!node.IsScalar()) return false;
        flags = static_cast<ImGuiHoveredFlags>(node.as<int>());
        return true;
    }

    // ---------- Main: Style <-> YAML Node ----------
    inline void StyleToYaml(const ImGuiStyle& style, YAML::Node& out) {
        // Font scaling
        out["FontSizeBase"] = style.FontSizeBase;
        out["FontScaleMain"] = style.FontScaleMain;
        out["FontScaleDpi"] = style.FontScaleDpi;

        // General
        out["Alpha"] = style.Alpha;
        out["DisabledAlpha"] = style.DisabledAlpha;

        // Window
        out["WindowPadding"] = EncodeImVec2(style.WindowPadding);
        out["WindowRounding"] = style.WindowRounding;
        out["WindowBorderSize"] = style.WindowBorderSize;
        out["WindowBorderHoverPadding"] = style.WindowBorderHoverPadding;
        out["WindowMinSize"] = EncodeImVec2(style.WindowMinSize);
        out["WindowTitleAlign"] = EncodeImVec2(style.WindowTitleAlign);
        out["WindowMenuButtonPosition"] = EncodeImGuiDir(style.WindowMenuButtonPosition);

        // Child
        out["ChildRounding"] = style.ChildRounding;
        out["ChildBorderSize"] = style.ChildBorderSize;

        // Popup
        out["PopupRounding"] = style.PopupRounding;
        out["PopupBorderSize"] = style.PopupBorderSize;

        // Frame
        out["FramePadding"] = EncodeImVec2(style.FramePadding);
        out["FrameRounding"] = style.FrameRounding;
        out["FrameBorderSize"] = style.FrameBorderSize;

        // Spacing / Layout
        out["ItemSpacing"] = EncodeImVec2(style.ItemSpacing);
        out["ItemInnerSpacing"] = EncodeImVec2(style.ItemInnerSpacing);
        out["CellPadding"] = EncodeImVec2(style.CellPadding);
        out["TouchExtraPadding"] = EncodeImVec2(style.TouchExtraPadding);
        out["IndentSpacing"] = style.IndentSpacing;
        out["ColumnsMinSpacing"] = style.ColumnsMinSpacing;
        out["ScrollbarSize"] = style.ScrollbarSize;
        out["ScrollbarRounding"] = style.ScrollbarRounding;
        out["ScrollbarPadding"] = style.ScrollbarPadding;
        out["GrabMinSize"] = style.GrabMinSize;
        out["GrabRounding"] = style.GrabRounding;
        out["LogSliderDeadzone"] = style.LogSliderDeadzone;

        // Images
        out["ImageRounding"] = style.ImageRounding;
        out["ImageBorderSize"] = style.ImageBorderSize;

        // Tabs
        out["TabRounding"] = style.TabRounding;
        out["TabBorderSize"] = style.TabBorderSize;
        out["TabMinWidthBase"] = style.TabMinWidthBase;
        out["TabMinWidthShrink"] = style.TabMinWidthShrink;
        out["TabCloseButtonMinWidthSelected"] = style.TabCloseButtonMinWidthSelected;
        out["TabCloseButtonMinWidthUnselected"] = style.TabCloseButtonMinWidthUnselected;
        out["TabBarBorderSize"] = style.TabBarBorderSize;
        out["TabBarOverlineSize"] = style.TabBarOverlineSize;

        // Tables
        out["TableAngledHeadersAngle"] = style.TableAngledHeadersAngle;
        out["TableAngledHeadersTextAlign"] = EncodeImVec2(style.TableAngledHeadersTextAlign);

        // Tree lines
        out["TreeLinesFlags"] = EncodeImGuiTreeNodeFlags(style.TreeLinesFlags);
        out["TreeLinesSize"] = style.TreeLinesSize;
        out["TreeLinesRounding"] = style.TreeLinesRounding;

        // Drag & Drop
        out["DragDropTargetRounding"] = style.DragDropTargetRounding;
        out["DragDropTargetBorderSize"] = style.DragDropTargetBorderSize;
        out["DragDropTargetPadding"] = style.DragDropTargetPadding;

        // Color marker
        out["ColorMarkerSize"] = style.ColorMarkerSize;
        out["ColorButtonPosition"] = EncodeImGuiDir(style.ColorButtonPosition);

        // Text alignment
        out["ButtonTextAlign"] = EncodeImVec2(style.ButtonTextAlign);
        out["SelectableTextAlign"] = EncodeImVec2(style.SelectableTextAlign);

        // Separator
        out["SeparatorSize"] = style.SeparatorSize;
        out["SeparatorTextBorderSize"] = style.SeparatorTextBorderSize;
        out["SeparatorTextAlign"] = EncodeImVec2(style.SeparatorTextAlign);
        out["SeparatorTextPadding"] = EncodeImVec2(style.SeparatorTextPadding);

        // Display / safe area
        out["DisplayWindowPadding"] = EncodeImVec2(style.DisplayWindowPadding);
        out["DisplaySafeAreaPadding"] = EncodeImVec2(style.DisplaySafeAreaPadding);

        // Mouse cursor scale
        out["MouseCursorScale"] = style.MouseCursorScale;

        // Antialiasing
        out["AntiAliasedLines"] = style.AntiAliasedLines;
        out["AntiAliasedLinesUseTex"] = style.AntiAliasedLinesUseTex;
        out["AntiAliasedFill"] = style.AntiAliasedFill;

        // Tessellation
        out["CurveTessellationTol"] = style.CurveTessellationTol;
        out["CircleTessellationMaxError"] = style.CircleTessellationMaxError;

        // Behaviors
        out["HoverStationaryDelay"] = style.HoverStationaryDelay;
        out["HoverDelayShort"] = style.HoverDelayShort;
        out["HoverDelayNormal"] = style.HoverDelayNormal;
        out["HoverFlagsForTooltipMouse"] = EncodeImGuiHoveredFlags(style.HoverFlagsForTooltipMouse);
        out["HoverFlagsForTooltipNav"] = EncodeImGuiHoveredFlags(style.HoverFlagsForTooltipNav);

        // Internal (optional but keep)
        out["_MainScale"] = style._MainScale;
        out["_NextFrameFontSizeBase"] = style._NextFrameFontSizeBase;

        // Colors
        YAML::Node colorsNode;
        for (int i = 0; i < ImGuiCol_COUNT; ++i) {
            ImGuiCol colEnum = static_cast<ImGuiCol>(i);
            colorsNode[EncodeImGuiCol(colEnum)] = EncodeImVec4(style.Colors[i]);
        }
        out["Colors"] = colorsNode;
    }

    inline bool YamlToStyle(const YAML::Node& node, ImGuiStyle& style) {
        if (!node.IsMap()) return false;

        // Helper to read scalar
        auto readFloat = [&](const char* key, float& out) {
            if (node[key]) out = node[key].as<float>();
            };
        auto readBool = [&](const char* key, bool& out) {
            if (node[key]) out = node[key].as<bool>();
            };
        auto readVec2 = [&](const char* key, ImVec2& out) {
            if (node[key]) DecodeImVec2(node[key], out);
            };
        auto readDir = [&](const char* key, ImGuiDir& out) {
            if (node[key]) DecodeImGuiDir(node[key], out);
            };
        auto readFlagsTree = [&](const char* key, ImGuiTreeNodeFlags& out) {
            if (node[key]) DecodeImGuiTreeNodeFlags(node[key], out);
            };
        auto readFlagsHover = [&](const char* key, ImGuiHoveredFlags& out) {
            if (node[key]) DecodeImGuiHoveredFlags(node[key], out);
            };

        // Font
        readFloat("FontSizeBase", style.FontSizeBase);
        readFloat("FontScaleMain", style.FontScaleMain);
        readFloat("FontScaleDpi", style.FontScaleDpi);

        readFloat("Alpha", style.Alpha);
        readFloat("DisabledAlpha", style.DisabledAlpha);

        readVec2("WindowPadding", style.WindowPadding);
        readFloat("WindowRounding", style.WindowRounding);
        readFloat("WindowBorderSize", style.WindowBorderSize);
        readFloat("WindowBorderHoverPadding", style.WindowBorderHoverPadding);
        readVec2("WindowMinSize", style.WindowMinSize);
        readVec2("WindowTitleAlign", style.WindowTitleAlign);
        readDir("WindowMenuButtonPosition", style.WindowMenuButtonPosition);

        readFloat("ChildRounding", style.ChildRounding);
        readFloat("ChildBorderSize", style.ChildBorderSize);

        readFloat("PopupRounding", style.PopupRounding);
        readFloat("PopupBorderSize", style.PopupBorderSize);

        readVec2("FramePadding", style.FramePadding);
        readFloat("FrameRounding", style.FrameRounding);
        readFloat("FrameBorderSize", style.FrameBorderSize);

        readVec2("ItemSpacing", style.ItemSpacing);
        readVec2("ItemInnerSpacing", style.ItemInnerSpacing);
        readVec2("CellPadding", style.CellPadding);
        readVec2("TouchExtraPadding", style.TouchExtraPadding);
        readFloat("IndentSpacing", style.IndentSpacing);
        readFloat("ColumnsMinSpacing", style.ColumnsMinSpacing);
        readFloat("ScrollbarSize", style.ScrollbarSize);
        readFloat("ScrollbarRounding", style.ScrollbarRounding);
        readFloat("ScrollbarPadding", style.ScrollbarPadding);
        readFloat("GrabMinSize", style.GrabMinSize);
        readFloat("GrabRounding", style.GrabRounding);
        readFloat("LogSliderDeadzone", style.LogSliderDeadzone);

        readFloat("ImageRounding", style.ImageRounding);
        readFloat("ImageBorderSize", style.ImageBorderSize);

        readFloat("TabRounding", style.TabRounding);
        readFloat("TabBorderSize", style.TabBorderSize);
        readFloat("TabMinWidthBase", style.TabMinWidthBase);
        readFloat("TabMinWidthShrink", style.TabMinWidthShrink);
        readFloat("TabCloseButtonMinWidthSelected", style.TabCloseButtonMinWidthSelected);
        readFloat("TabCloseButtonMinWidthUnselected", style.TabCloseButtonMinWidthUnselected);
        readFloat("TabBarBorderSize", style.TabBarBorderSize);
        readFloat("TabBarOverlineSize", style.TabBarOverlineSize);

        readFloat("TableAngledHeadersAngle", style.TableAngledHeadersAngle);
        readVec2("TableAngledHeadersTextAlign", style.TableAngledHeadersTextAlign);

        readFlagsTree("TreeLinesFlags", style.TreeLinesFlags);
        readFloat("TreeLinesSize", style.TreeLinesSize);
        readFloat("TreeLinesRounding", style.TreeLinesRounding);

        readFloat("DragDropTargetRounding", style.DragDropTargetRounding);
        readFloat("DragDropTargetBorderSize", style.DragDropTargetBorderSize);
        readFloat("DragDropTargetPadding", style.DragDropTargetPadding);

        readFloat("ColorMarkerSize", style.ColorMarkerSize);
        readDir("ColorButtonPosition", style.ColorButtonPosition);

        readVec2("ButtonTextAlign", style.ButtonTextAlign);
        readVec2("SelectableTextAlign", style.SelectableTextAlign);

        readFloat("SeparatorSize", style.SeparatorSize);
        readFloat("SeparatorTextBorderSize", style.SeparatorTextBorderSize);
        readVec2("SeparatorTextAlign", style.SeparatorTextAlign);
        readVec2("SeparatorTextPadding", style.SeparatorTextPadding);

        readVec2("DisplayWindowPadding", style.DisplayWindowPadding);
        readVec2("DisplaySafeAreaPadding", style.DisplaySafeAreaPadding);

        readFloat("MouseCursorScale", style.MouseCursorScale);

        readBool("AntiAliasedLines", style.AntiAliasedLines);
        readBool("AntiAliasedLinesUseTex", style.AntiAliasedLinesUseTex);
        readBool("AntiAliasedFill", style.AntiAliasedFill);

        readFloat("CurveTessellationTol", style.CurveTessellationTol);
        readFloat("CircleTessellationMaxError", style.CircleTessellationMaxError);

        readFloat("HoverStationaryDelay", style.HoverStationaryDelay);
        readFloat("HoverDelayShort", style.HoverDelayShort);
        readFloat("HoverDelayNormal", style.HoverDelayNormal);
        readFlagsHover("HoverFlagsForTooltipMouse", style.HoverFlagsForTooltipMouse);
        readFlagsHover("HoverFlagsForTooltipNav", style.HoverFlagsForTooltipNav);

        readFloat("_MainScale", style._MainScale);
        readFloat("_NextFrameFontSizeBase", style._NextFrameFontSizeBase);

        // Colors
        if (node["Colors"] && node["Colors"].IsMap()) {
            YAML::Node colorsNode = node["Colors"];
            for (auto it = colorsNode.begin(); it != colorsNode.end(); ++it) {
                std::string key = it->first.as<std::string>();
                ImGuiCol colEnum;
                if (DecodeImGuiCol(YAML::Node(key), colEnum)) {
                    DecodeImVec4(it->second, style.Colors[colEnum]);
                }
            }
        }
        return true;
    }

} // namespace ImGuiYaml