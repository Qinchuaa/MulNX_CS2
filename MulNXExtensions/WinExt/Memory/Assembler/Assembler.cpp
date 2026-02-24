#include "Assembler.hpp"

using namespace MulNX::Memory;

Asm::Code&& Asm::Assembler::Release() {
    return std::move(this->Asm);
}

Asm::RegInfo Asm::get_reg_info(Reg r) {
    return reg_infos[static_cast<size_t>(r)];
}

void Asm::Assembler::emit_byte(uint8_t byte) {
    this->Asm.code.push_back(byte);
}

Asm::Assembler& Asm::Assembler::mov(Reg dst, Reg src) {
    auto dst_info = get_reg_info(dst);
    auto src_info = get_reg_info(src);

    // 基础 REX 前缀，W=1（64位操作）
    uint8_t rex_byte = 0x48;
    if (src_info.is_ext) rex_byte |= 0x04;  // REX.R
    if (dst_info.is_ext) rex_byte |= 0x01;  // REX.B
    emit_byte(rex_byte);

    emit_byte(0x89); // mov r64, r64 操作码

    uint8_t modrm = 0xC0 | ((src_info.code & 7) << 3) | (dst_info.code & 7);
    emit_byte(modrm);
    return *this;
}
Asm::Assembler& Asm::Assembler::mov(Reg dst, uint64_t imm) {
    auto dst_info = get_reg_info(dst);

    // REX 前缀：W=1（64位操作），如果目标寄存器是扩展寄存器则设置 B 位
    uint8_t rex_byte = 0x48;                     // 基础 REX.W=1
    if (dst_info.is_ext) {
        rex_byte |= 0x01;                         // REX.B 位
    }
    emit_byte(rex_byte);

    // 操作码：0xB8 + 寄存器的低3位
    uint8_t opcode = 0xB8 + (dst_info.code & 7);
    emit_byte(opcode);

    // 立即数（8字节，小端序）
    for (int i = 0; i < 8; ++i) {
        emit_byte((imm >> (i * 8)) & 0xFF);
    }
    return *this;
}

Asm::Assembler& Asm::Assembler::call(Mem m) {
    auto reg_info = get_reg_info(m.reg);

    // 如果基址是扩展寄存器（R8-R15），需要添加 REX 前缀（REX.B=1）
    if (reg_info.is_ext) {
        // REX 前缀：0100 0001 (0x41) 表示 REX.W=0, REX.R=0, REX.X=0, REX.B=1
        emit_byte(0x41);
    }

    // 操作码 0xFF
    emit_byte(0xFF);

    // ModRM 字节：mod=00 (无位移), reg=010 (表示 call 操作), rm=基址寄存器的低3位
    uint8_t modrm = (0 << 6) | (2 << 3) | (reg_info.code & 7);
    emit_byte(modrm);

    return *this;
}

Asm::Assembler& Asm::Assembler::nop() {
    emit_byte(0x90);
    return *this;
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