#include "EntityList.hpp"

#include <MulNXExtensions/WinExt/WinExt.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

uintptr_t C_EntityList::Address = 0;

// CS角色信息获取流程：
// 1. 检测LocalController是否存在并获取
// 2. 通过LocalController获取LocalHandlePawn（实体句柄）
// 3. 获取dwEntityList（实体列表基址）
// 4. 通过LocalHandlePawn获取LocalPawn（本地玩家实体）
// 5. 遍历实体列表：通过dwEntityList获取其他玩家的Controller
// 6. 通过Controller获取其Handle
// 7. 通过Handle获取对应的Pawn
// 8. 从Pawn读取各种游戏数据

// 索引结构说明：
// int类型32位索引的位分配：
//   [31:9] (高23位) - 段索引（Segment Index），用于定位内存中的地址段
//   [8:0]  (低9位)  - 段内偏移（Segment Offset），用于在地址段内定位具体对象

// 根据索引获取对应的内存地址段
// 注意：多个实体索引可能共享同一个地址段（当它们的高23位相同时）
uintptr_t C_EntityList::GetEntityBaseFromIndex(int Index) {
    // 通过索引的高23位（Index >> 9）计算段基址在dwEntityList中的偏移
    // 公式：目标地址 = dwEntityList + (段索引 * 8) + 16
    return MulNX::Memory::Read<uintptr_t>(C_EntityList::Address + 0x8 * (Index >> 9) + 0x10);
}

// 根据索引获取Controller对象
// 注意：索引的低9位决定了在地址段内的具体位置
uintptr_t C_EntityList::GetEntityControllerFromIndex(int Index) {
    // 获取该索引对应的内存地址段
    uintptr_t entitylistbase = GetEntityBaseFromIndex(Index);
    if (!entitylistbase) return 0;
    // 使用索引的低9位（Index & 0x1FF）计算段内偏移
    // 每个Controller占用0x78字节，通过乘法定位具体位置
    return MulNX::Memory::Read<uintptr_t>(entitylistbase + (0x70 * (Index & 0x1FF)));
}

// 根据实体句柄(uHandle)获取Pawn对象
uintptr_t C_EntityList::GetEntityPawnFromHandle(uint32_t uHandle) {
    // 实体句柄的低15位包含有效索引信息
    // 注意：0x7FFF = 32767 (二进制15个1)，提取低15位
    const int nIndex = uHandle & 0x7FFF;
    // 使用提取的索引获取对应的内存地址段
    uintptr_t entitylistbase = GetEntityBaseFromIndex(nIndex);
    if (!entitylistbase) return 0;
    // 使用索引的低9位计算段内偏移（与获取Controller方式相同）
    // Pawn对象同样占用0x78字节空间
    return MulNX::Memory::Read<uintptr_t>(entitylistbase + (0x70 * (nIndex & 0x1FF)));
}



C_Entity& C_EntityList::at(size_t index) {
    std::shared_lock lock(this->EntityListMtx);
    return Entitys.at(index);
}
size_t C_EntityList::size() const {
    std::shared_lock lock(this->EntityListMtx);
    return Entitys.size();
}
bool C_EntityList::empty() const {
    std::shared_lock lock(this->EntityListMtx);
    return Entitys.empty();
}
C_Entity C_EntityList::GetEntity(size_t Index) {
    std::shared_lock lock(this->EntityListMtx);
    return this->Entitys[Index];
}

void C_EntityList::Update() {
    std::unique_lock lock(this->EntityListMtx);


    for (int i = 0; i < 64; ++i) {
        C_Entity& Entity = this->Entitys[i];
        Entity.Controller.Address = this->GetEntityControllerFromIndex(i);//这里其实只访问了第一个地址段
        if (!Entity.Controller.Address)continue;

        Entity.IndexInEntityList = i;
        Entity.Controller.hPawn = MulNX::Memory::Read<uint32_t>(Entity.Controller.Address + cs2_dumper::schemas::client_dll::CBasePlayerController::m_hPawn);
        if (Entity.Controller.hPawn == 0xFFFFFFFF)continue;
        Entity.Pawn.Address = this->GetEntityPawnFromHandle(Entity.Controller.hPawn);
        if (!Entity.Pawn.Address)continue;
        Entity.Pawn.m_iTeamNum = MulNX::Memory::Read<int>(Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
        Entity.Pawn.m_iHealth = MulNX::Memory::Read<int>(Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
        Entity.Pawn.m_vOldOrigin = MulNX::Memory::Read<DirectX::XMFLOAT3>(Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
        Entity.Pawn.m_iHideHUD = MulNX::Memory::Read<uint32_t>(Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_iHideHUD);
        Entity.Pawn.m_angEyeAngles = MulNX::Memory::Read<DirectX::XMFLOAT3>(Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_angEyeAngles);
        //获取GameSecneNode
        Entity.Pawn.GameSceneNode.Address = MulNX::Memory::Read<uintptr_t>(Entity.Pawn.Address + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
        //获取位置和视角信息
        Entity.Pawn.GameSceneNode.Position = MulNX::Memory::Read<DirectX::XMFLOAT3>(Entity.Pawn.GameSceneNode.Address + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecAbsOrigin);
        Entity.Pawn.GameSceneNode.RotationEuler = MulNX::Memory::Read<DirectX::XMFLOAT3>(Entity.Pawn.GameSceneNode.Address + cs2_dumper::schemas::client_dll::CGameSceneNode::m_angAbsRotation);

        MulNX::Memory::ReadString(Entity.Controller.Address + cs2_dumper::schemas::client_dll::CBasePlayerController::m_iszPlayerName,
            Entity.Controller.m_iszPlayerName, sizeof(Entity.Controller.m_iszPlayerName));
    }
}