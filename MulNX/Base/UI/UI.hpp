#pragma once

#include <MulNXThirdParty/imgui_d11/imgui.h>
#include <MulNXThirdParty/imgui_d11/imgui_stdlib.h>
#include <atomic>

namespace MulNX {
    namespace UI {
        bool SliderFloat(const char* label, std::atomic<float>& av, float v_min, float v_max, const char* format, ImGuiSliderFlags flags);
        bool SliderInt(const char* label, std::atomic<int>& av, int v_min, int v_max, const char* format, ImGuiSliderFlags flags);
    }
}