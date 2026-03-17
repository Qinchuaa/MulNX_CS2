#include "UI.hpp"

bool MulNX::UI::SliderFloat(const char* label, std::atomic<float>& av, float v_min, float v_max, const char* format, ImGuiSliderFlags flags) {
    float v = av.load(std::memory_order_relaxed);
    bool changed = ImGui::SliderFloat(label, &v, v_min, v_max, format, flags);
    if (changed) {
        av.store(v, std::memory_order_relaxed);
    }
    return changed;
}
bool MulNX::UI::SliderInt(const char* label, std::atomic<int>& av, int v_min, int v_max, const char* format, ImGuiSliderFlags flags) {
    int v = av.load(std::memory_order_relaxed);
    bool changed = ImGui::SliderInt(label, &v, v_min, v_max, format, flags);
    if (changed) {
        av.store(v, std::memory_order_relaxed);
    }
    return changed;
}