#include "Logger.hpp"

#include <MulNX/Core/Core.hpp>
#include <MulNX/Systems/PathManager/PathManager.hpp>

bool MulNX::Logger::Init() {
    this->logPath = this->ISys().PathManager()->PathGetForShared("Log") / ("Log_" + this->Core->GetName() + ".txt");
    this->target = std::ofstream(this->logPath, std::ios::out | std::ios::app | std::ios::binary);
    if (!this->target) {
        MulNX::ErrorTerminate("Cannot Wirte Log!");
    }
    target << I18n("log.new") << std::endl;
    this->SendTask("Loging", [this]()->bool {
        this->Log();
        return true;
        });
    return true;
}

void MulNX::Logger::Log() {
    std::string line;
    while (this->logs.try_dequeue(line)) {
        this->target << line << std::endl;
    }
}