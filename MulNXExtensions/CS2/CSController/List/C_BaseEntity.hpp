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
    };

    class C_ClassInfo {
    public:
        char* pName() { return *reinterpret_cast<char**>(reinterpret_cast<uintptr_t>(this) + 0x20); }
    };

    class C_BaseEntity {
    public:
        template<typename T>
        T* As() { return reinterpret_cast<T*>(this); }

        C_ClassInfo* pClassInfo() { return *reinterpret_cast<C_ClassInfo**>(reinterpret_cast<uintptr_t>(this) + 0x10); }
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

    };

    class C_BaseCombatCharacter :public C_BaseFlex {
    public:
        
    };

    class C_BasePlayerPawn :public C_BaseCombatCharacter {
    public:
        DirectX::XMFLOAT3* vOldOrigin() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin); }
        //DirectX::XMFLOAT3 GetEyePos(){}
    };

    
    class CBasePlayerController :public C_BaseEntity {
    public:

        CHandle<C_BasePlayerPawn>* hPawn() { return reinterpret_cast<CHandle<C_BasePlayerPawn>*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CBasePlayerController::m_hPawn); }
    };
}