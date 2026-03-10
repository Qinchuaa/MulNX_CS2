#pragma once

#include "../Assembler/Assembler.hpp"

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
            // using CallBack = void(*)(HookEx*, RegContext*);
            bool Enable = true;
            uint8_t* Target = nullptr;
            void* pCaller = nullptr;
            MulNX::Memory::Asm::Code CodeCaller{};
            std::vector<std::function<void(RegContext*)>> Callbacks;
            std::shared_mutex MutexEx;
            std::vector<uint8_t> RawCmd;
        private:
            static void Dispatch(HookEx* pHookExInstance, RegContext* Ctx);
        public:
            HookEx() = default;
            static std::unique_ptr<HookEx> Create(uint8_t* Target, int Len);
            void AddCallback(std::function<void(RegContext*)>&& Callback);
            void Attach();

        };
    }
}