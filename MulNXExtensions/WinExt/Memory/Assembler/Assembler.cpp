#include "Assembler.hpp"

using namespace MulNX::Memory;

Asm::Code&& Asm::Assembler::Release() {
    return std::move(this->Asm);
}

Asm::RegInfo Asm::get_reg_info(Reg r) {
    return reg_infos[static_cast<size_t>(r)];
}

int Asm::get_mod_and_disp(int64_t disp, int& bytes) {
    if (disp == 0) {
        bytes = 0;
        return 0;
    }
    else if (disp >= -128 && disp <= 127) {
        bytes = 1;
        return 1;
    }
    else {
        bytes = 4;
        return 2;
    }
}

void Asm::Assembler::emit_byte(uint8_t byte) {
    this->Asm.code.push_back(byte);
}

size_t Asm::Code::Size()const {
    return this->code.size();
}

uint8_t* Asm::Code::Data() {
    return this->code.data();
}

const uint8_t* Asm::Code::Data() const {
    return this->code.data();
}
Asm::Code& Asm::Code::push_back(std::vector<uint8_t>&& Code) {
    this->code.insert(this->code.end(),
        std::make_move_iterator(Code.begin()),
        std::make_move_iterator(Code.end()));
    return *this;
}
Asm::Code& Asm::Code::push_back(Asm::Code&& other) {
    this->push_back(std::move(other.code));
    return *this;
}