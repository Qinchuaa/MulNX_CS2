#pragma once

#include <MulNX/MulNX.hpp>
struct DemoHelperPrivateData {
	std::vector<float> TimeMarks{};
};

class DemoHelper final :public MulNX::ModuleBase {
private:
    std::vector<float>Marks{};
    std::atomic<MulNX::any_unique_ptr*>* ppUpdateData = nullptr;
    MulNXHandle hUINode{};
    std::atomic<std::shared_ptr<DemoHelperPrivateData>> Data = nullptr;
public:

    bool Init()override;

    void VirtualMain()override;
    void ProcessMsg(MulNX::Message& Msg)override;
    bool UINodeFunc(MulNXUINode* node);
    //void HandleUICommand(MulNXMessage* Msg);

    bool MarkTime();
    bool JumpMark(size_t Index);
};