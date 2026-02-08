#include"Controller.hpp"

std::ostringstream C_PlayerController::GetMsg()const {
	std::ostringstream oss;
	oss << "   名称:   " << this->m_iszPlayerName;
	return oss;
}