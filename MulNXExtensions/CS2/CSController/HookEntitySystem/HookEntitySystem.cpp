#include "HookEntitySystem.hpp"

#include <MulNXExtensions/CS2/CSController/CSController.hpp>

bool HookEntitySystem::Init() {

    static auto vtable = (uint8_t**)IVClass::Assume(this->CS2()->Modules.client.dwGameEntitySystem())->GetVTablePtr();
    auto pAddEntity = vtable[15];

    this->hkAddEntity = MulNX::Hook::Create(pAddEntity,
        0, false, [this](RegContext* ctx, MulNX::Hook* hk)->bool {
            auto pEntity = *ctx->P2<CS2::C_BaseEntity*>();
            MulNX::Message msg("Game/Entity/Added"_hash);
            msg.p1.as<CS2::C_BaseEntity*>() = pEntity;
            this->ISys().PublishAsync(std::move(msg));
            return true;
        }).value();
    this->hkAddEntity->Attach();
    this->ISys().LogSucc("实体添加钩子已部署");

    auto pRemoveEntity = vtable[16];
    this->hkRemoveEntity = MulNX::Hook::Create(pRemoveEntity,
        0, false, [this](RegContext* ctx, MulNX::Hook* hk)->bool {
            auto pEntity = *ctx->P2<CS2::C_BaseEntity*>();
            MulNX::Message msg("Game/Entity/Removed"_hash);
            msg.p1.as<CS2::C_BaseEntity*>() = pEntity;
            this->ISys().PublishAsync(std::move(msg));
            return true;
        }).value();
    this->hkRemoveEntity->Attach();
    this->ISys().LogSucc("实体移除钩子已部署");

    return true;
}