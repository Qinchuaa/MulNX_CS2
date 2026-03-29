#pragma once 

#include <stdfloat>
#include <MulNX/Base/Math/Math.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>

using GameTime_t = float;

namespace CS2 {
    class CViewSetup {
    public:
        // 定位关键数据
        int* pWidth() { return reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x434); }
        int* pHeight() { return reinterpret_cast<int*>(reinterpret_cast<uintptr_t>(this) + 0x43C); }

        float* pFov() { return reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + 0x498); }
        DirectX::XMFLOAT3* pViewOrigin() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + 0x4a0); }
        DirectX::XMFLOAT3* pViewAngles() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + 0x4b8); }
    };

    class CHandleBase {
    public:
        uint32_t handle;
        CHandleBase(uint32_t handle) :handle(handle) {}
        int GetIndexInEntityList() { return this->handle & 0x7FFF; }
        bool Valid() { return this->handle != 0xFFFFFFFF; }

        CHandleBase() :handle(0xFFFFFFFF) {}
    };

    template<typename T>
    class CHandle :public CHandleBase {
    public:
        
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

    class BoneArray {
        constexpr static int32_t unkSize = 32;
    public:
        DirectX::XMFLOAT3* at(int32_t index) { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + index * unkSize); }
    };

    class CSkeletonInstance :public CGameSceneNode {
        constexpr static uintptr_t unkSchema = 0x80;
    public:
        BoneArray** unkBoneArray() { return reinterpret_cast<BoneArray**>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState + unkSchema); }
    };

    class C_BaseEntity {
    public:
        template<typename T>
            requires std::derived_from<T, CS2::C_BaseEntity>
        T* As() { return reinterpret_cast<T*>(this); }

        C_ClassInfo** pClassInfo() { return reinterpret_cast<C_ClassInfo**>(reinterpret_cast<uintptr_t>(this) + 0x10); }
        CGameSceneNode** pGameSceneNode() { return reinterpret_cast<CGameSceneNode**>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode); }
        int32_t* iHealth() { return reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth); }
        uint8_t* iTeamNum() { return reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum); }

        DirectX::XMFLOAT3 GetBonePos(int index);
    };

    class C_SoundEventEntity :public C_BaseEntity {
    public:
        
    };

    class CGlowProperty{
    public:
        void* fGlowColor() { return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGlowProperty::m_fGlowColor); }
        int32_t* iGlowType() { return reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGlowProperty::m_iGlowType); }
        int32_t* iGlowTeam() { return reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGlowProperty::m_iGlowTeam); }
        int32_t* nGlowRange() { return reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGlowProperty::m_nGlowRange); }
        int32_t* nGlowRangeMin() { return reinterpret_cast<int32_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGlowProperty::m_nGlowRangeMin); }
        void* glowColorOverride() { return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGlowProperty::m_glowColorOverride); }
        bool* bFlashing() { return reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGlowProperty::m_bFlashing); }
        float* flGlowTime() { return reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGlowProperty::m_flGlowTime); }
        float* flGlowStartTime() { return reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGlowProperty::m_flGlowStartTime); }
        bool* bEligibleForScreenHighlight() { return reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGlowProperty::m_bEligibleForScreenHighlight); }
        bool* bGlowing() { return reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CGlowProperty::m_bGlowing); }
    };

    class C_BaseModelEntity :public C_BaseEntity {
    public:
        CGlowProperty* Glow() { return reinterpret_cast<CGlowProperty*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_Glow); }
        DirectX::XMFLOAT3* vecViewOffset() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BaseModelEntity::m_vecViewOffset); }
    };

    class CBaseAnimGraph :public C_BaseModelEntity {
    public:

    };

    class C_PlantedC4 :public CBaseAnimGraph{
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
    public:
        
    };

    class C_CSWeaponBase :public C_BasePlayerWeapon {
    public:

    };

    class CPlayer_WeaponServices :public CPlayerPawnComponent {
    public:
        CHandle<C_BasePlayerWeapon>* hActiveWeapon() { return reinterpret_cast<CHandle<C_BasePlayerWeapon>*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CPlayer_WeaponServices::m_hActiveWeapon); }
    };

    using ObserverMode_t = uint32_t;
    class CPlayer_ObserverServices :public CPlayerPawnComponent {
    public:
        uint8_t* iObserverMode() { return reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_iObserverMode); }
        CHandle<C_BaseEntity>* hObserverTarget() { return reinterpret_cast<CHandle<C_BaseEntity>*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_hObserverTarget); }
        ObserverMode_t* iObserverLastMode() { return reinterpret_cast<ObserverMode_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_iObserverLastMode); }
        bool* bForcedObserverMode() { return reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_bForcedObserverMode); }
        float* flObserverChaseDistance() { return reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_flObserverChaseDistance); }
        GameTime_t* flObserverChaseDistanceCalcTime() { return reinterpret_cast<GameTime_t*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::CPlayer_ObserverServices::m_flObserverChaseDistanceCalcTime); }
    };

    class C_BasePlayerPawn :public C_BaseCombatCharacter {
    public:
        CPlayer_ObserverServices** pObserverServices() { return reinterpret_cast<CPlayer_ObserverServices**>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pObserverServices); }
        DirectX::XMFLOAT3* vOldOrigin() { return reinterpret_cast<DirectX::XMFLOAT3*>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin); }
        //DirectX::XMFLOAT3 GetEyePos(){}
        CPlayer_WeaponServices** pWeaponServices() { return reinterpret_cast<CPlayer_WeaponServices**>(reinterpret_cast<uintptr_t>(this) + cs2_dumper::schemas::client_dll::C_BasePlayerPawn::m_pWeaponServices); }


        CHandle<C_BasePlayerWeapon> GetHandleActiveWeapon();
        CHandle<C_BaseEntity> GetHandleObserverTarget();
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

    class CCSPlayerController :public CBasePlayerController {
    public:

    };
}