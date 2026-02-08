#pragma once

#include <DirectXMath.h>
#include <string>

// 扩展 DirectX 命名空间
namespace DirectX {
    // 自由函数扩展
    inline float Dot(const XMFLOAT3& a, const XMFLOAT3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline XMFLOAT3 Cross(const XMFLOAT3& a, const XMFLOAT3& b) {
        return XMFLOAT3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    inline float Length(const XMFLOAT3& v) {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    inline XMFLOAT3 Normalize(const XMFLOAT3& v) {
        float len = Length(v);
        if (len > 0.0f) {
            return XMFLOAT3(v.x / len, v.y / len, v.z / len);
        }
        return v;
    }

    // 运算符重载
    inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b) {
        return XMFLOAT3(a.x + b.x, a.y + b.y, a.z + b.z);
    }

    inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b) {
        return XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z);
    }

    inline XMFLOAT3 operator*(const XMFLOAT3& v, const float scalar) {
        return XMFLOAT3(v.x * scalar, v.y * scalar, v.z * scalar);
    }

    inline XMFLOAT3 operator*(float scalar, const XMFLOAT3& v) {
        return v * scalar;
    }
    inline XMFLOAT3 operator*(XMFLOAT3& a, const XMFLOAT3& b) {
        return XMFLOAT3(a.x * b.x, a.y * b.y, a.z * b.z);
    }
    inline XMFLOAT3 operator/(const XMFLOAT3& v, const float scalar) {
        return XMFLOAT3(v.x / scalar, v.y / scalar, v.z / scalar);
    }
    inline XMFLOAT3& operator+=(XMFLOAT3& a, const XMFLOAT3& b) {
        a.x += b.x;
        a.y += b.y;
        a.z += b.z;
        return a;
    }
    inline XMFLOAT3& operator-=(XMFLOAT3& a, const XMFLOAT3& b) {
        a.x -= b.x;
        a.y -= b.y;
        a.z -= b.z;
        return a;
    }
    inline XMFLOAT3& operator*=(XMFLOAT3& v, const XMFLOAT3& b) {
        v.x *= b.x;
        v.y *= b.y;
        v.z *= b.z;
        return v;
    }
    inline XMFLOAT3& operator*=(XMFLOAT3& v, const float scalar) {
        v.x *= scalar;
        v.y *= scalar;
        v.z *= scalar;
        return v;
    }
    inline XMFLOAT3& operator/=(XMFLOAT3& v, const XMFLOAT3& b) {
        v.x /= b.x;
        v.y /= b.y;
        v.z /= b.z;
        return v;
    }
    inline XMFLOAT3& operator/=(XMFLOAT3& v, const float scalar) {
        v.x /= scalar;
        v.y /= scalar;
        v.z /= scalar;
        return v;
    }

    // 实用函数
    inline float Distance(const XMFLOAT3& a, const XMFLOAT3& b) {
        XMFLOAT3 diff = a - b;
        return Length(diff);
    }

    inline XMFLOAT3 Lerp(const XMFLOAT3& a, const XMFLOAT3& b, float t) {
        t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
        return a + (b - a) * t;
    }

    inline std::string to_string(const XMFLOAT3& v) {
        return "X: " + std::to_string(v.x) + "  Y: " + std::to_string(v.y) + "  Z: " + std::to_string(v.z);
    }
}

const std::string Line_ = "+------------------------------------------------+";
const std::string Gap_ = "     ";

namespace MulNX {
    namespace Base {
        namespace Math {
            //基本空间状态，包括坐标和旋转（四元数）
            class SpatialState {
            public:
                DirectX::XMVECTOR PositionAndFOV{};
                DirectX::XMVECTOR RotationQuat{};

                DirectX::XMFLOAT3 GetPosition()const;
                DirectX::XMFLOAT4 GetRotationQuat()const;
                DirectX::XMFLOAT3 GetRotationEuler()const;
                float GetFOV()const;

                DirectX::XMFLOAT4 GetPositionAndFOV()const;

                std::string GetMsg()const;

                bool operator==(const SpatialState& other) const {
                    const float epsilon = 1e-6f;

                    DirectX::XMFLOAT4 pos1, pos2, rot1, rot2;
                    DirectX::XMStoreFloat4(&pos1, PositionAndFOV);
                    DirectX::XMStoreFloat4(&pos2, other.PositionAndFOV);
                    DirectX::XMStoreFloat4(&rot1, RotationQuat);
                    DirectX::XMStoreFloat4(&rot2, other.RotationQuat);

                    return std::abs(pos1.x - pos2.x) < epsilon &&
                        std::abs(pos1.y - pos2.y) < epsilon &&
                        std::abs(pos1.z - pos2.z) < epsilon &&
                        std::abs(pos1.w - pos2.w) < epsilon &&
                        std::abs(rot1.x - rot2.x) < epsilon &&
                        std::abs(rot1.y - rot2.y) < epsilon &&
                        std::abs(rot1.z - rot2.z) < epsilon &&
                        std::abs(rot1.w - rot2.w) < epsilon;
                }
            };

            //关键帧,是时间、空间状态、FOV、景深的组合
            class CameraKeyFrame {
            public:
                SpatialState SpatialState{};

                float KeyTime{};
                int Depth{ 0 };

                std::string GetMsg()const;
            };

            //帧，包含渲染一帧的所有预备前置条件
            class Frame {
            public:
                //目标空间状态，自由摄像机轨道完全使用，锁定摄像机轨道使用视角，不使用其它
                SpatialState SpatialState{};

                //目标OB模式，4是自由摄像机轨道和锁定摄像机轨道，2是第一人称摄像机轨道
                uint8_t TargetOBMode = 4;
                //第一人称摄像机轨道和锁定摄像机轨道使用，均指代目标人物
                uint8_t TargetPlayerIndexInMap = 0;

                float GameSpeed{};

                std::string GetMsg()const;

                DirectX::XMFLOAT3 GetPosition()const {
                    return this->SpatialState.GetPosition();
                }
                DirectX::XMFLOAT4 GetRotationQuat()const {
                    return this->SpatialState.GetRotationQuat();
                }
                DirectX::XMFLOAT3 GetRotationEuler()const {
                    return this->SpatialState.GetRotationEuler();
                }
                float GetFOV()const {
                    return this->SpatialState.GetFOV();
                }

                DirectX::XMFLOAT4 GetPositionAndFOV()const {
                    return this->SpatialState.GetPositionAndFOV();
                }

                bool operator==(const Frame&)const = default;
            };

            void CSEulerToQuat(const DirectX::XMFLOAT3& Euler, DirectX::XMFLOAT4& QuaT);
            DirectX::XMVECTOR CSEulerToQuatVec(const DirectX::XMFLOAT3& Euler);
            void CSQuatToEuler(const DirectX::XMFLOAT4& Quat, DirectX::XMFLOAT3& Euler);
            //只修改Pitch和Yaw，不修改Roll
            void CSDirToEuler(const DirectX::XMFLOAT3& Dir, DirectX::XMFLOAT3& Euler);
            bool XMWorldToScreen(const DirectX::XMFLOAT3& pWorldPos, DirectX::XMFLOAT2& pScreenPos, const float* pMatrixPtr, const float pWinWidth, const float pWinHeight);

            DirectX::XMFLOAT3 RotatePoint(
                const DirectX::XMFLOAT3& inputPoint,
                float pitchDegrees,  // 绕Y轴旋转（俯仰）
                float yawDegrees,    // 绕Z轴旋转（偏航）
                float rollDegrees    // 绕X轴旋转（滚转）
            );

            //移动点
            bool MovePoint(DirectX::XMFLOAT3& SourcePoint, const DirectX::XMFLOAT3& TargetPoint);

            class DOFParam {
            public:
                float NearBlurry;
                float NearCrisp;
                float FarCrisp;
                float FarBlurry;
            };
            //计算景深参数
            void CalculateDOFParameters(float FocusDistance, float CrispRadius, float BlurDistance, DOFParam& DOFParam);
        };
    }
};