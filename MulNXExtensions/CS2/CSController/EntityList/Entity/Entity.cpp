#include"Entity.hpp"

std::ostringstream C_Entity::GetMsg()const {
	std::ostringstream oss;
	oss << this->Pawn.GetMsg().str() << this->Controller.GetMsg().str();
	return oss;
}