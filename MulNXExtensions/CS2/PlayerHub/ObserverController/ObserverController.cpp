#include "ObserverController.hpp"
#include <MulNXExtensions/CS2/CSController/CSController.hpp>
#include <atomic>
#include <chrono>
#include <thread>

bool ObserverController::Init() {
    this->ISys()
        .SubscribeAsync("CameraSystem/Play/Started")
        .SubscribeAsync("CameraSystem/Play/Ended")
        .SubscribeAsync("spec_mode_changed_to");

    this->SendTask("CSControl", [this]() -> bool {
        this->Main();
        return true;
        });

    return true;
}

void ObserverController::ProcessMsg(MulNX::Message& Msg) {
    switch (Msg.type) {
    case "CameraSystem/Play/Started"_hash: {
        this->ISys().LogInfo("收到运镜开始消息");
        this->HandlePlayStarted();
        break;
    }
    case "CameraSystem/Play/Ended"_hash: {
        this->ISys().LogInfo("收到运镜结束消息");
        this->HandlePlayEnded();
        break;
    }
    case "spec_mode_changed_to"_hash: {
        uint8_t newMode = Msg.p1.low<uint8_t>();
        this->OnSpecModeChanged(newMode);
        break;
    }
    }
}

void ObserverController::Main() {
    this->UpdateObserverState();  // 轮询检测模式变化，并发布事件
    this->EntryProcessMsg();      // 处理消息队列（包括 spec_mode_changed_to）
}

void ObserverController::UpdateObserverState() {
    try {
        auto localPlayerPawn = this->CS2()->Modules.client.GetLocalPlayerPawn();
        if (!localPlayerPawn)return;
        auto pObserverServices = MulNX::MRead(localPlayerPawn->pObserverServices());
        if (!pObserverServices)return;
        uint8_t detectedMode = MulNX::MRead(pObserverServices->iObserverMode());
        static uint8_t lastObservedSpecMode = detectedMode;
        if (lastObservedSpecMode != detectedMode) {
            MulNX::Message msg("spec_mode_changed_to"_hash);
            msg.p1.low<uint8_t>() = detectedMode;
            this->ISys().PublishAsync(std::move(msg));
            lastObservedSpecMode = detectedMode;
        }
    }
    catch (const std::exception& e) {
        this->ISys().LogError(std::format("UpdateObserverState error: {}", e.what()));
    }
}

bool ObserverController::HandlePlayStarted() {
    if (this->CampathPlaying) {
        this->ISys().LogWarning("已在运镜播放中，重复的播放开始消息将被忽略。");
        return false;
    }
    this->startedAsSpecMode = this->currentMode;
    this->CampathPlaying = true;

    if (this->currentMode != 4) {   // 不是自由视角，需要切换
        this->ISys().LogInfo("运镜开始时当前模式非 spec_mode 4，尝试切换并等待生效。");
        this->CS2()->ExecuteCommand("spec_mode 4");
        this->waitingForSpecMode4 = true;
    }
    else {
        this->ISys().LogInfo("运镜开始时已是 spec_mode 4，直接开始播放。");
        this->waitingForSpecMode4 = false;
    }
    return true;
}

bool ObserverController::HandlePlayEnded() {
    if (startedAsSpecMode == 2) {
        this->ISys().LogInfo("运镜结束后恢复 spec_mode 2。");
        this->CS2()->ExecuteCommand("spec_mode 2");
    }
    this->CampathPlaying = false;
    this->waitingForSpecMode4 = false;
    this->startedAsSpecMode = 0;
    return true;
}

void ObserverController::OnSpecModeChanged(uint8_t newMode) {
    this->currentMode = newMode;
    // 等待 spec_mode 4 生效阶段
    if (this->waitingForSpecMode4) {
        if (newMode == 4) {
            this->ISys().LogInfo("spec_mode 4 已确认进入，切换生效。");
            this->waitingForSpecMode4 = false;
        }
        // 还在等待，不做其他处理
        return;
    }

    // 运镜播放中，检测用户是否切回 spec_mode 2
    if (this->CampathPlaying && newMode == 2) {
        this->ISys().LogWarning("检测到 spec_mode 2，已中断当前运镜。");
        this->ISys().PublishAsync("CameraSystem/Play/Shutdown"_hash);
    }
}