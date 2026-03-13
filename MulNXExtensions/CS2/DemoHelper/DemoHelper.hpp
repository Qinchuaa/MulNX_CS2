#pragma once

#include <MulNX/MulNX.hpp>

class MulNXUINode;
class TripleBufferBase;

class DemoHelper final :public MulNX::ModuleBase {
private:
    std::vector<float>Marks{};
    std::atomic<MulNX::any_unique_ptr*>* ppUpdateData = nullptr;
    MulNXHandle hUINode{};
public:
    DemoHelper() : ModuleBase() {};

    bool Init()override;

    void VirtualMain()override;
    void ProcessMsg(MulNX::Message* Msg)override;

    void HandleUICommand(MulNX::Message* Msg);
    //void HandleUICommand(MulNXMessage* Msg);

    bool MarkTime();
    bool JumpMark(size_t Index);
};