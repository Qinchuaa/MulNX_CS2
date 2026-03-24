#include "Config.hpp"
#include <MulNX/Base/CharUtility/CharUtility.hpp>
#include <MulNX/Base/UI/UI.hpp>
#include <cstdlib>
#include <Windows.h>
#include <sstream>

void MulNX::ErrorTerminate(const std::string& Msg,
    const std::source_location& loc) {
    std::ostringstream oss{};

    oss << "致命错误："
        << "\n\n";

    oss << "错误描述："
        << Msg
        << "\n\n";

    oss << "发生于："
        << "\n文件: " << loc.file_name()
        << "\n函数: " << loc.function_name()
        << "\n行号: " << loc.line()
        << "\n列号: " << loc.column()
        << "\n\n";

    oss << "MulNX将在您点击确定后关闭当前进程";

    std::string Full = oss.str();
    std::wstring wFull = MulNX::Base::CharUtility::U8ToW(Full);

    MessageBoxW(nullptr, wFull.c_str(), L"MulNX 错误中断！", MB_OK);
    std::terminate();
}

void MulNX::SetUIStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // 基础参数：圆角、边框、间距
    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 8.0f;
    style.ChildRounding = 4.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);   // 标题居中
    style.FrameBorderSize = 1.0f;
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.WindowPadding = ImVec2(10.0f, 8.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);

    // ============================================================
    // 深暖灰背景 + 暗粉按钮 + 暗紫标签页 (适配版)
    // ============================================================

    // 1. 背景：更深的暖灰褐
    colors[ImGuiCol_WindowBg] = ImVec4(0.20f, 0.18f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.22f, 0.20f, 0.18f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.24f, 0.22f, 0.20f, 1.00f);

    // 2. 文本：浅色（保证深背景可读）
    colors[ImGuiCol_Text] = ImVec4(0.96f, 0.94f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.65f, 0.62f, 0.58f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.85f, 0.60f, 0.75f, 0.45f);   // 暗粉高亮

    // 3. 标题栏 & 菜单栏 (略深于背景)
    colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.14f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.24f, 0.22f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.16f, 0.14f, 0.12f, 0.70f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.18f, 0.16f, 0.14f, 1.00f);

    // 4. 框架背景 (输入框、滑块等)
    colors[ImGuiCol_FrameBg] = ImVec4(0.42f, 0.39f, 0.36f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.50f, 0.46f, 0.42f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.58f, 0.54f, 0.50f, 1.00f);

    // 5. 按钮 – 暗粉色系 (加深，与字体拉开对比)
    colors[ImGuiCol_Button] = ImVec4(0.70f, 0.45f, 0.55f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.80f, 0.55f, 0.65f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.60f, 0.35f, 0.45f, 1.00f);

    // 6. 复选框、单选框、滑块 – 适配暗粉色系
    colors[ImGuiCol_CheckMark] = ImVec4(0.85f, 0.60f, 0.70f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.55f, 0.65f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.90f, 0.65f, 0.75f, 1.00f);

    // 7. 头部 (树节点、选择项) – 暗粉紫，与按钮呼应
    colors[ImGuiCol_Header] = ImVec4(0.65f, 0.50f, 0.70f, 0.65f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.75f, 0.60f, 0.80f, 0.85f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.70f, 0.55f, 0.75f, 0.95f);

    // 8. 分隔线
    colors[ImGuiCol_Separator] = ImVec4(0.55f, 0.50f, 0.46f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.75f, 0.65f, 0.60f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.85f, 0.75f, 0.70f, 1.00f);

    // 9. 滚动条
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.30f, 0.28f, 0.26f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.52f, 0.49f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.65f, 0.62f, 0.58f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.75f, 0.72f, 0.68f, 1.00f);

    // 10. 标签页 (Tab) – 暗紫色系
    colors[ImGuiCol_Tab] = ImVec4(0.45f, 0.30f, 0.60f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.55f, 0.40f, 0.70f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.60f, 0.45f, 0.75f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.40f, 0.26f, 0.55f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.50f, 0.35f, 0.65f, 1.00f);

    // 11. 表格 – 暖黄/米色系，微调以匹配整体
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.48f, 0.44f, 0.40f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.28f, 0.26f, 0.24f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.32f, 0.30f, 0.27f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.52f, 0.48f, 0.44f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.62f, 0.58f, 0.54f, 1.00f);

    // 12. 绘图 – 保持多色，稍调粉/紫氛围
    colors[ImGuiCol_PlotLines] = ImVec4(0.80f, 0.90f, 0.65f, 1.00f);   // 草绿
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 1.00f, 0.75f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.85f, 0.60f, 0.80f, 1.00f);   // 淡紫
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.95f, 0.70f, 0.90f, 1.00f);

    // 13. 拖放目标 & 导航 – 调整与暗粉/暗紫协调
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.85f, 0.60f, 0.75f, 0.80f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.75f, 0.55f, 0.85f, 0.80f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.90f, 0.70f, 0.90f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.08f, 0.06f, 0.04f, 0.50f);

    // 14. 模态窗口遮罩
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.08f, 0.06f, 0.04f, 0.60f);

    // 15. 边框
    colors[ImGuiCol_Border] = ImVec4(0.58f, 0.54f, 0.50f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.08f, 0.06f, 0.04f, 0.50f);
}