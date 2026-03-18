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

using HookCallBack = void(*)(RegContext*);

struct HookInfo {
    int var1;
    int var2;
    HookCallBack callback;
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
            std::function<void(RegContext*)> callback;
            size_t threadNumInAsm = 0;

            void* pAsmDispatcher = nullptr;
            MulNX::Memory::Asm::Code dispatcherAsmCode{};
            
            uint8_t* hookTarget = nullptr;
            size_t overrideSize = 0;
            std::vector<uint8_t> hookTargetRawCode;
            MulNX::Memory::Asm::Code jumperAsmCode{};
        private:
            static void Dispatch(HookEx* pHookExInstance, RegContext* ctx);
        public:
            HookEx() = default;
            ~HookEx();
            static std::unique_ptr<HookEx> Create(uint8_t* Target, int Len, std::function<void(RegContext*)>&& callback);
            Result Attach();
            Result Detach();
        };
    }
}