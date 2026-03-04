#include "Hook.hpp"

#include <Windows.h>
#include "../Assembler/Assembler.hpp"
#include <atomic>

int MyAddable = 0;

void MulNX::Memory::HookEx::Dispatch(HookEx* pHookExInstance, RegContext* Ctx) {
    std::shared_lock lock(pHookExInstance->MutexEx);
    if (!pHookExInstance->Enable) {
        return;
    }
    const auto& CallbacksInstance = pHookExInstance->Callbacks;
    for (const auto& CallbackInstance : CallbacksInstance) {
        CallbackInstance(Ctx);
    }
    return;
}

MulNX::Memory::HookEx* MulNX::Memory::HookEx::Create(uint8_t* Target, int Len) {
    // 首先创建HookEx实例
    auto* HookExInstance = new HookEx();
    HookExInstance->Target = Target;

    // 复制覆盖处指令
    // 逆向分析已知是14字节
    HookExInstance->RawCmd = std::vector<uint8_t>(Target, Target + 14);

    // 为调度器汇编部分分配代码
    HookExInstance->pCaller = VirtualAlloc(0, 1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (HookExInstance->pCaller) {
        MessageBoxW(nullptr, L"内存分配成功", L"MulNX", 0);
    }

    // 创建Code对象
    MulNX::Memory::Asm::Code CodeCaller{};
    {
        // 创建编译器
        using enum MulNX::Memory::Asm::Reg;
        using namespace MulNX::Memory::Asm;
        Assembler Asm{};

        // 计算结构体大小（保证16字节对齐）
        const size_t ctxSize = (sizeof(RegContext) + 15) & ~15;

        // 分配栈空间
        Asm.sub(RSP, ctxSize);

        // 保存所有寄存器到 [rsp + offset]
        Asm.mov(Mem(RSP, offsetof(RegContext, rax)), RAX);
        Asm.mov(Mem(RSP, offsetof(RegContext, rcx)), RCX);
        Asm.mov(Mem(RSP, offsetof(RegContext, rdx)), RDX);
        Asm.mov(Mem(RSP, offsetof(RegContext, rbx)), RBX);
        Asm.mov(Mem(RSP, offsetof(RegContext, rsp)), RSP); // 保存当前rsp（分配后的）
        Asm.mov(Mem(RSP, offsetof(RegContext, rbp)), RBP);
        Asm.mov(Mem(RSP, offsetof(RegContext, rsi)), RSI);
        Asm.mov(Mem(RSP, offsetof(RegContext, rdi)), RDI);
        Asm.mov(Mem(RSP, offsetof(RegContext, r8)), R8);
        Asm.mov(Mem(RSP, offsetof(RegContext, r9)), R9);
        Asm.mov(Mem(RSP, offsetof(RegContext, r10)), R10);
        Asm.mov(Mem(RSP, offsetof(RegContext, r11)), R11);
        Asm.mov(Mem(RSP, offsetof(RegContext, r12)), R12);
        Asm.mov(Mem(RSP, offsetof(RegContext, r13)), R13);
        Asm.mov(Mem(RSP, offsetof(RegContext, r14)), R14);
        Asm.mov(Mem(RSP, offsetof(RegContext, r15)), R15);

        // 指令执行区
        {
            Asm
                .mov(RCX, (uintptr_t)HookExInstance)// 此参数对应 HookEx
                .mov(RDX, RSP)// 此参数对应 RegContext
                .mov(RAX, (uintptr_t)&MulNX::Memory::HookEx::Dispatch)
                .call(RAX);
        }

        // 恢复区
        Asm.mov(RAX, Mem(RSP, offsetof(RegContext, rax)));
        Asm.mov(RCX, Mem(RSP, offsetof(RegContext, rcx)));
        Asm.mov(RDX, Mem(RSP, offsetof(RegContext, rdx)));
        Asm.mov(RBX, Mem(RSP, offsetof(RegContext, rbx)));
        // 不恢复 rsp
        Asm.mov(RBP, Mem(RSP, offsetof(RegContext, rbp)));
        Asm.mov(RSI, Mem(RSP, offsetof(RegContext, rsi)));
        Asm.mov(RDI, Mem(RSP, offsetof(RegContext, rdi)));
        Asm.mov(R8, Mem(RSP, offsetof(RegContext, r8)));
        Asm.mov(R9, Mem(RSP, offsetof(RegContext, r9)));
        Asm.mov(R10, Mem(RSP, offsetof(RegContext, r10)));
        Asm.mov(R11, Mem(RSP, offsetof(RegContext, r11)));
        Asm.mov(R12, Mem(RSP, offsetof(RegContext, r12)));
        Asm.mov(R13, Mem(RSP, offsetof(RegContext, r13)));
        Asm.mov(R14, Mem(RSP, offsetof(RegContext, r14)));
        Asm.mov(R15, Mem(RSP, offsetof(RegContext, r15)));

        // 释放上下文空间
        Asm.add(RSP, ctxSize);


        // 临时：给RAX分配一个合法内存，让CS2不崩溃
        Asm
            .mov(RAX, (uintptr_t)&MyAddable);

        CodeCaller = Asm.Release();
    }

    {
        // 复制原始指令，等到集成反汇编引擎处理相对寻址
        //CodeCaller.push_back(std::move(Raw));
    }
    
    // 跳转到原处
    {
        using enum MulNX::Memory::Asm::Reg;
        using namespace MulNX::Memory::Asm;
        Assembler Asm{};
        Asm.nop().nop()
            .jmp64((uintptr_t)Target + 14)
            .nop()
            .nop()
            .nop();
        CodeCaller.push_back(std::move(Asm.Release()));
    }
    // 复制机器码到VirtualAlloc分配的内存
    memcpy(HookExInstance->pCaller, CodeCaller.Data(), CodeCaller.Size());
    return HookExInstance;
}

void MulNX::Memory::HookEx::AddCallback(std::function<void(RegContext*)>&& Callback) {
    std::unique_lock lock(this->MutexEx);
    this->Callbacks.push_back(std::move(Callback));
    return;
}

void MulNX::Memory::HookEx::Attach() {
    // 修改原指令位置
    MulNX::Memory::Asm::Code Code{};
    {
        using enum MulNX::Memory::Asm::Reg;
        using namespace MulNX::Memory::Asm;
        Assembler Asm{};

        Asm.jmp64((uintptr_t)this->pCaller);
        Code = Asm.Release();
    }

    // 覆盖原位置
    memcpy(this->Target, Code.Data(), Code.Size());
}