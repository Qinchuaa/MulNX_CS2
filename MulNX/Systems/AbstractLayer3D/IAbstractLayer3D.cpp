#include "IAbstractLayer3D.hpp"

MulNX::TimeBridge::TimeBridge(IAbstractLayer3D* pAL3D) : pAL3D(pAL3D) {
    this->startTime = std::chrono::steady_clock::now();
}

void MulNX::TimeBridge::update() {
    float time = this->pAL3D->GetTime();
    if (time > this->lastRealTime) {
        this->lastRealTime = time;
    }
    else if (this->lastRealTime - time > 0.025f) {
        this->lastRealTime = time;
    }
    return;
}

bool MulNX::TimeBridge::RefreshVirtual(bool virtualTimePlaying, float scale) {
    this->update();
    this->refreshTime = this->lastRealTime;
    this->startTime = std::chrono::steady_clock::now();
    this->scale = scale;
    this->virtualTimePlaying = virtualTimePlaying;
    return true;
}

float MulNX::TimeBridge::GetReal() {
    this->update();
    return this->lastRealTime;
}

bool MulNX::TimeBridge::JumpReal(float time) {
    return this->pAL3D->JumpTime(time);
}

float MulNX::TimeBridge::GetVirtual() {
    // 这里不需要更新，因为虚拟时间的更新是由RefreshVirtual控制的，GetVirtual只负责计算当前的虚拟时间
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<float>(now - this->startTime).count();
    return this->refreshTime + elapsed * this->scale;
}

float MulNX::TimeBridge::Get() {
    return this->virtualTimePlaying ? this->GetVirtual() : this->GetReal();
}


MulNX::TimeBridge* MulNX::IAbstractLayer3D::Time() {
    return &this->timeBridge;
}