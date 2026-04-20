#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>

namespace MulNX {
    namespace Task {
        class Worker {
        public:
            std::thread t;
            std::vector<std::function<bool()>>tasks;
            std::atomic<std::function<bool()>*>entry;
            void Start();
        };

        class RegistrationPacket {
        public:
            std::string targetWorker;
            std::function<bool()>task;
        };
    }
    class TaskSystem final :public ModuleBase {
        std::unordered_map<std::string, std::unique_ptr<Task::Worker>>workers{};
        void HandleAddTask(MulNX::Message& msg);
    public:
        bool Init()override;
        void ProcessMsg(MulNX::Message& msg)override;
        void Main();
    };
}