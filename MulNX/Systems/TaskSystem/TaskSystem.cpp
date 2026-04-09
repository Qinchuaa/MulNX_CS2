#include "TaskSystem.hpp"

#include <MulNX/Systems/MessageManager/IMessageManager.hpp>

void MulNX::Task::Worker::Start() {
    this->t = std::thread([this]() {
        for (;;) {
            if (this->entry.load(std::memory_order_acquire)) {
                this->tasks.push_back(std::move(*this->entry.load(std::memory_order_acquire)));
                this->entry.store(nullptr, std::memory_order_release);
            }
            for (const auto& task : this->tasks) {
                task();
            }
        }
        });
    return;
}

bool MulNX::TaskSystem::Init() {
    this->ISys()
        .SubscribeAsync("Task/Create");
    return true;
}

void MulNX::TaskSystem::ProcessMsg(MulNX::Message& msg) {
    switch (msg.type) {
    case "Task/Create"_hash: {
        this->ISys().LogInfo("接收到一个任务！");
        this->HandleAddTask(msg);
        break;
    }
    default: {
        break;
    }
    }
}

void MulNX::TaskSystem::VirtualMain() {
    this->EntryProcessMsg();
    
}

void MulNX::TaskSystem::HandleAddTask(MulNX::Message& msg) {
    auto pRegistrationPacket = msg.asp.get<MulNX::Task::RegistrationPacket>();
    auto it = this->workers.find(pRegistrationPacket->targetWorker);
    if (it == this->workers.end()) {
        this->workers[pRegistrationPacket->targetWorker] = std::make_unique<MulNX::Task::Worker>();
        this->workers[pRegistrationPacket->targetWorker]->Start();
        this->ISys().LogSucc(std::format("成功创建工作者：{}", pRegistrationPacket->targetWorker));
    }
    this->workers[pRegistrationPacket->targetWorker]->entry.store(&pRegistrationPacket->task, std::memory_order_release);
    while (this->workers[pRegistrationPacket->targetWorker]->entry.load(std::memory_order_acquire) != nullptr) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    this->ISys().LogSucc(std::format("成功将任务添加进入工作者：{}", pRegistrationPacket->targetWorker));
}