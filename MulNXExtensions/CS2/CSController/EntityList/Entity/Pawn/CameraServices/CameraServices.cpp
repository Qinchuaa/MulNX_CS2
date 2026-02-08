#include"CameraServices.hpp"

std::ostringstream C_CameraServices::GetMsg()const {
	std::ostringstream oss;
	oss << " FOV:" << this->iFOV;
	return oss;
}