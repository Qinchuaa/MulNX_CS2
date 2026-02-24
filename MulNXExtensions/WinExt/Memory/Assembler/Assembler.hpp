#pragma once

#include <vector>
#include <cstdint>

namespace MulNX {
    namespace Memory {
        namespace Asm {
            enum class Reg : uint8_t {
                RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
                R8, R9, R10, R11, R12, R13, R14, R15
            };

            class Mem {
            public:
                Reg reg;
                Mem() = delete;
                Mem(Reg reg) {
                    this->reg = reg;
                }
            };

            struct RegInfo {
                uint8_t code;    // 硬件编码 (0-15)
                bool is_ext;     // 是否扩展寄存器 (R8-R15)
            };

            constexpr RegInfo reg_infos[] = {
                {0, false},  // RAX
                {1, false},  // RCX
                {2, false},  // RDX
                {3, false},  // RBX
                {4, false},  // RSP
                {5, false},  // RBP
                {6, false},  // RSI
                {7, false},  // RDI
                {8, true},   // R8
                {9, true},   // R9
                {10, true},  // R10
                {11, true},  // R11
                {12, true},  // R12
                {13, true},  // R13
                {14, true},  // R14
                {15, true},  // R15
            };

            RegInfo get_reg_info(Reg r);

            class Code {
                std::vector<uint8_t> code;
                friend class Assembler;
            public:
                size_t Size()const;
                uint8_t* Data();
                const uint8_t* Data() const;       // const accessor for read-only users

                Code() = default;
            };

            class Assembler {
                Code Asm{};
                void emit_byte(uint8_t byte);
            public:
                Assembler& mov(Reg R1, Reg R2);
                Assembler& mov(Mem M1, Reg R2);
                Assembler& mov(Reg R1, Mem M2);
                Assembler& mov(Reg dst, uint64_t imm);
                Assembler& call(Reg R);
                Assembler& call(Mem R);
                Assembler& nop();

                Code&& Release();
            };
        }
    }
}