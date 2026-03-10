#include "MulNXMessage.hpp"

void MulNX::Message::Clear() {
	this->Type = 0;
	this->SubType = 0;
	this->Handle = MulNXHandle{};
	this->ParamInt = 0;
	this->ParamFloat = 0;
	this->pMsgChannel = nullptr;

	return;
}