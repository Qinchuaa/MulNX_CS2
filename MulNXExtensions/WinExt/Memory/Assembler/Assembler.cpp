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


Asm::Assembler& Asm::Assembler::call(Reg r) {
    auto reg_info = get_reg_info(r);
    if (reg_info.is_ext) {
        emit_byte(0x41); // REX.B=1 (处理 R8~R15)
    }
    emit_byte(0xFF);      // 操作码
    // ModRM: mod=11 (寄存器直接), reg=010 (call), r/m=寄存器的低3位
    uint8_t modrm = (3 << 6) | (2 << 3) | (reg_info.code & 7);
    emit_byte(modrm);
    return *this;
}
Asm::Assembler& Asm::Assembler::call(Mem m) {
    auto reg_info = get_reg_info(m.base);

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

Asm::Assembler& Asm::Assembler::push(Reg r) {
    auto reg_info = get_reg_info(r);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0x50 + (reg_info.code & 7));    // PUSH r64 短编码
    return *this;
}

Asm::Assembler& Asm::Assembler::push(Mem m) {
    auto reg_info = get_reg_info(m.base);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0xFF);                           // 操作码
    uint8_t modrm = (0 << 6) | (6 << 3) | (reg_info.code & 7); // mod=00, reg=6 (push), rm
    emit_byte(modrm);
    return *this;
}

Asm::Assembler& Asm::Assembler::pop(Reg r) {
    auto reg_info = get_reg_info(r);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0x58 + (reg_info.code & 7));    // POP r64 短编码
    return *this;
}

Asm::Assembler& Asm::Assembler::pop(Mem m) {
    auto reg_info = get_reg_info(m.base);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0x8F);                           // 操作码
    uint8_t modrm = (0 << 6) | (0 << 3) | (reg_info.code & 7); // mod=00, reg=0 (pop), rm
    emit_byte(modrm);
    return *this;
}

Asm::Assembler& Asm::Assembler::jmp(Reg r) {
    auto reg_info = get_reg_info(r);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0xFF);                           // 操作码
    uint8_t modrm = (3 << 6) | (4 << 3) | (reg_info.code & 7); // mod=11, reg=4 (jmp), rm
    emit_byte(modrm);
    return *this;
}

Asm::Assembler& Asm::Assembler::jmp(Mem m) {
    auto reg_info = get_reg_info(m.base);
    if (reg_info.is_ext) {
        emit_byte(0x41);                     // REX.B=1
    }
    emit_byte(0xFF);                           // 操作码
    uint8_t modrm = (0 << 6) | (4 << 3) | (reg_info.code & 7); // mod=00, reg=4 (jmp), rm
    emit_byte(modrm);
    return *this;
}

// JMP rel32：操作码 0xE9 + 4字节偏移（小端序）
Asm::Assembler& Asm::Assembler::jmp(int32_t rel_offset) {
    emit_byte(0xE9); // 近跳转操作码
    // 写入4字节有符号偏移量（小端序）
    for (int i = 0; i < 4; ++i) {
        emit_byte((rel_offset >> (i * 8)) & 0xFF);
    }
    return *this;
}

// JMP rel8：操作码 0xEB + 1字节偏移
Asm::Assembler& Asm::Assembler::jmp_rel8(int8_t rel_offset) {
    emit_byte(0xEB); // 短跳转操作码
    emit_byte(static_cast<uint8_t>(rel_offset));
    return *this;
}
Asm::Assembler& Asm::Assembler::jmp64(uint64_t target) {
    // 生成 jmp qword ptr [rip+0] : FF 25 00 00 00 00
    emit_byte(0xFF);           // 操作码
    emit_byte(0x25);           // ModRM: mod=00, reg=4 (jmp), rm=101 (RIP相对)
    emit_byte(0x00);           // 偏移量低字节 (disp32 = 0)
    emit_byte(0x00);
    emit_byte(0x00);
    emit_byte(0x00);
    // 将目标地址作为 8 字节立即数附加在指令后
    for (int i = 0; i < 8; ++i) {
        emit_byte((target >> (i * 8)) & 0xFF);
    }
    return *this;
}