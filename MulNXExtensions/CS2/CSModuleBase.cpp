#include "CSModuleBase.hpp"
#include <MulNXExtensions/CS2/PlayerHub/PlayerHub.hpp>

PlayerHub* CSModuleBase::Hub() {
    static PlayerHub* playerHub = this->Core->ModuleManager()->FindModule<PlayerHub>("PlayerHub");
    return playerHub;
}