#include"GameSceneNode.hpp"

std::ostringstream C_GameSceneNode::GetMsg()const {
	std::ostringstream oss;
	oss << "   坐标: X:" << this->Position.x << "   Y:" << this->Position.y << "   Z:" << this->Position.z;
	oss << "   角度: X:" << this->RotationEuler.x << "   Y:" << this->RotationEuler.y << "   Z:" << this->RotationEuler.z;

	return oss;
}