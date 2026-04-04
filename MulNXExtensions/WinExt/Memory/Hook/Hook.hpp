#pragma once

#include <MulNXExtensions/WinExt/Memory/Assembler/Assembler.hpp>

#include <cstdint>
#include <functional>
#include <vector>
#include <shared_mutex>

struct RegContext {
    uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
};

namespace MulNX {
    namespace Memory {
        class HookEx {
            enum class Result :uint8_t {
                Attached,
                AttachSuccess,
                AttachError,

                Detached,
                DetachSuccess,
                DetachError
            };

            const static size_t allocSize = 1000;

            bool attached = false;
            std::function<bool(RegContext*, HookEx*)> callback;
            size_t threadNumInAsm = 0;

            void* pAsmDispatcher = nullptr;
            MulNX::Memory::Asm::Code dispatcherAsmCode{};
            
            uint8_t* hookTarget = nullptr;
            size_t overrideSize = 0;
            MulNX::Memory::Asm::Code hookTargetRawCode;
            MulNX::Memory::Asm::Code jumperAsmCode{};
        private:
            static std::vector<uint8_t> FixRIPRelativeInstructions(const std::vector<uint8_t>& raw_code,
                uintptr_t old_base, uintptr_t new_base);
            static uintptr_t Dispatch(HookEx* pHookExInstance, RegContext* ctx);

            uintptr_t jmpTarget0 = 0;
            uintptr_t jmpTarget1 = 0;
        public:
            uintptr_t pMaybeRawFunc = 0;// 可能的原函数地址（如果覆盖的指令是一个完整函数的开头）
            HookEx() = default;
            ~HookEx();
            static std::unique_ptr<HookEx> Create(uint8_t* Target, int Len, bool extraStackAdjust, std::function<bool(RegContext*, HookEx*)>&& callback);
            Result Attach();
            Result Detach();
        };
    }
}