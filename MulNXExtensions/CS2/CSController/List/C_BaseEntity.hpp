#pragma once 

#include <MulNX/Base/Math/Math.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

namespace CS2 {
    template<typename T>
    class CHandle {
    public:
        uint32_t handle;
        CHandle(uint32_t handle) :handle(handle) {}
        int GetIndexInEntityList() { return this->handle & 0x7FFF; }
        bool Valid() { return this->handle != 0xFFFFFFFF; }
    };

    class C_ClassInfo {
    public:
        char** pName() { return reinterpret_cast<char**>(reinterpret_cast<uintptr_t>(this) + 0x20); }
    };

    class CGameSceneNode {
    public:
        DirectX::XMFLOAT3* vecAbsOrigin() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecAbsOrigin); }
        DirectX::XMFLOAT3* vecOrigin() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecOrigin); }
        DirectX::XMFLOAT3* angAbsRotation() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGameSceneNode::m_angAbsRotation); }
        DirectX::XMFLOAT3* angRotation() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGameSceneNode::m_angRotation); }
        DirectX::XMFLOAT3* vecWrappedLocalOrigin() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecWrappedLocalOrigin); }
        DirectX::XMFLOAT3* angWrappedLocalRotation() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGameSceneNode::m_angWrappedLocalRotation); }
        DirectX::XMFLOAT3* vRenderOrigin() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vRenderOrigin); }
        bool* bDebugAbsOriginChanges() { return reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGameSceneNode::m_bDebugAbsOriginChanges); }
    };

    class C_BaseEntity {
    public:
        template<typename T>
        requires std::derived_from<T, CS2::C_BaseEntity>
        T* As() { return reinterpret_cast<T*>(this); }

        C_ClassInfo** pClassInfo() { return reinterpret_cast<C_ClassInfo**>(reinterpret_cast<uintptr_t>(this) + 0x10); }
        CGameSceneNode** pGameSceneNode() { return reinterpret_cast<CGameSceneNode**>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode); }
    };

    class C_BaseModelEntity :public C_BaseEntity {
    public:
        DirectX::XMFLOAT3* vecViewOffset() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_vecViewOffset); }
    };

    class CBaseAnimGraph :public C_BaseModelEntity {
    public:

    };

    class C_BaseFlex :public CBaseAnimGraph {
    public:

    };

    class C_BaseGrenade :public C_BaseFlex {
    public:

    };

    class C_BaseCSGrenadeProjectile :public C_BaseGrenade {
    public:

    };

    class C_SmokeGrenadeProjectile :public C_BaseCSGrenadeProjectile {
    public:
        int32_t* nSmokeEffectTickBegin() { return reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_SmokeGrenadeProjectile::m_nSmokeEffectTickBegin); }
        bool* bDidSmokeEffect() { return reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_SmokeGrenadeProjectile::m_bDidSmokeEffect); }
        bool* bSmokeEffectSpawned() { return reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_SmokeGrenadeProjectile::m_bSmokeEffectSpawned); }
        DirectX::XMFLOAT3* vSmokeColor() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_SmokeGrenadeProjectile::m_vSmokeColor); }

    };

    class C_BaseCombatCharacter :public C_BaseFlex {
    public:
        
    };

    class C_EconEntity :public C_BaseFlex {
    public:

    };

    class CPlayerPawnComponent {
    public:
        
    };

    class C_BasePlayerWeapon :public C_EconEntity {

    };

    class CPlayer_WeaponServices :public CPlayerPawnComponent {
    public:
        CHandle<C_BasePlayerWeapon>* hActiveWeapon() { return reinterpret_cast<CHandle<C_BasePlayerWeapon>*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CPlayer_WeaponServices::m_hActiveWeapon); }
    };

    class C_BasePlayerPawn :public C_BaseCombatCharacter {
    public:
        DirectX::XMFLOAT3* vOldOrigin() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin); }
        //DirectX::XMFLOAT3 GetEyePos(){}
        CPlayer_WeaponServices** pWeaponServices() { return reinterpret_cast<CPlayer_WeaponServices**>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pWeaponServices); }
    };

    class C_CSPlayerPawnBase :public C_BasePlayerPawn {
    public:

    };

    
    class C_CSPlayerPawn :public C_CSPlayerPawnBase {
    public:
        DirectX::XMFLOAT3* angEyeAngles() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_angEyeAngles); }
    };

    class CBasePlayerController :public C_BaseEntity {
    public:

        CHandle<C_BasePlayerPawn>* hPawn() { return reinterpret_cast<CHandle<C_BasePlayerPawn>*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CBasePlayerController::m_hPawn); }
    };
}