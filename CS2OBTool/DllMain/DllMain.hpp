#pragma once

#include <MulNX/MulNX.hpp>
#include <array>

DWORD MulNX_CS2_Start(void*);
class ProjectManager;
class ElementManager;
class SolutionManager;

class MainDraw final:public MulNX::ModuleBase{
public:
    enum class KeybindRecordTarget {
        None,
        Element,
        Solution
    };

    std::string SelectedKeybindProjectName{};
    KeybindRecordTarget RecordingTarget = KeybindRecordTarget::None;
    std::string RecordingItemName{};
    unsigned char RecordingMainKey = 0;
    MulNX::KeyCheckPack PendingRecordedKeybind{};
    std::array<bool, 256> RecordingPrevKeys{};

    bool Init();
    void Window(MulNX::UINode* node);
    void RenderKeybindPage(ProjectManager* projectManager, ElementManager* elementManager, SolutionManager* solutionManager);
    void StartKeybindRecording(KeybindRecordTarget target, const std::string& itemName);
    void StopKeybindRecording();
    bool UpdateKeybindRecording(MulNX::KeyCheckPack& outBinding);
};
