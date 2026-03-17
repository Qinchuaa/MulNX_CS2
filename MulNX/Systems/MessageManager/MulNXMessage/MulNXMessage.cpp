#include "MulNXMessage.hpp"

void MulNX::Message::Clear() {
    this->type = 0;
    this->asp = MulNX::any_shared_ptr();
    this->p1.i = 0;
	this->p1.i = 0;

	return;
}