#include "Math.hpp"

#include <sstream>
#include <iomanip>
#include <format>

DirectX::XMFLOAT3 MulNX::Math::CameraKeyframe::GetPosition()const {
	DirectX::XMFLOAT3 position;
	DirectX::XMStoreFloat3(&position, this->PositionAndFOV);
	return position;
}

DirectX::XMFLOAT4 MulNX::Math::CameraKeyframe::GetRotationQuat() const {
	DirectX::XMFLOAT4 Quat;
	DirectX::XMStoreFloat4(&Quat, this->RotationQuat);
	return Quat;
}

float MulNX::Math::CameraKeyframe::GetFOV() const {
	return DirectX::XMVectorGetW(this->PositionAndFOV);
}

DirectX::XMFLOAT3 MulNX::Math::CameraKeyframe::GetRotationEuler()const {
	DirectX::XMFLOAT3 Euler;
	CSQuatToEuler(this->GetRotationQuat(), Euler);
	return Euler;
}

DirectX::XMFLOAT4 MulNX::Math::CameraKeyframe::GetPositionAndFOV()const {
	DirectX::XMFLOAT4 PositionAndFOV;
	DirectX::XMStoreFloat4(&PositionAndFOV, this->PositionAndFOV);
	return PositionAndFOV;
}

MulNX::Math::DOFParam MulNX::Math::CameraKeyframe::GetDOF()const {
    DirectX::XMFLOAT4 dof;
    DirectX::XMStoreFloat4(&dof, this->dof);
    return { dof.x,dof.y,dof.z,dof.w };
}

DirectX::XMVECTOR MulNX::Math::View::ToPositionAndFOV() {
    return DirectX::XMVectorSet(
        this->position.x,
        this->position.y,
        this->position.z,
        this->FOV
    );
}
DirectX::XMVECTOR MulNX::Math::View::ToRotationQuat() {
    return MulNX::Math::CSEulerToQuatVec(this->rotation);
}
DirectX::XMVECTOR MulNX::Math::View::ToDOFPack() {
    return DirectX::XMVectorSet(
        this->dof.NearBlurry,
        this->dof.NearCrisp,
        this->dof.FarCrisp,
        this->dof.FarBlurry
    );
}

std::string MulNX::Math::CameraKeyframe::GetMsg()const {
    DirectX::XMFLOAT4 PositionAndFOV = this->GetPositionAndFOV();
    DirectX::XMFLOAT3 Euler = this->GetRotationEuler();
    auto dof = this->GetDOF();
    std::ostringstream oss;

    oss << std::fixed << std::setprecision(6)
        << "  时间： " << this->time
        << "  坐标： X：" << PositionAndFOV.x
        << "  Y：" << PositionAndFOV.y
        << "  Z：" << PositionAndFOV.z
        << "  FOV:" << PositionAndFOV.w
        << "  角度： 俯仰：" << Euler.x
        << "  偏航：" << Euler.y
        << "  滚转：" << Euler.z
        << "  景深： 近模糊：" << dof.NearBlurry
        << "  近清晰：" << dof.NearCrisp
        << "  远清晰：" << dof.FarCrisp
        << "  远模糊：" << dof.FarBlurry;
    return oss.str();
}