#include "Assembler.hpp"

using namespace MulNX::Memory;

Asm::Code Asm::Assembler::Release() {
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
    this->Asm.push_back(byte);
}