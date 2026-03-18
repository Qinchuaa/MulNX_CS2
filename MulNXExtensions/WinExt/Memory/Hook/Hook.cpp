#include "Hook.hpp"

#include <MulNX/Config/Config.hpp>
#include <MulNXExtensions/WinExt/WinException/WinException.hpp>
#include <Windows.h>
#include <atomic>

int MyAddable = 0;

void MulNX::Memory::HookEx::Dispatch(HookEx* pHookExInstance, RegContext* ctx) {
    pHookExInstance->callback(ctx);
    return;
}

std::unique_ptr<MulNX::Memory::HookEx> MulNX::Memory::HookEx::Create(uint8_t* Target, int Len, std::function<void(RegContext*)>&& callback) {
    // 首先创建HookEx实例
    auto HookExInstance = std::make_unique<HookEx>();
    HookExInstance->hookTarget = Target;
    HookExInstance->callback = std::move(callback);
    HookExInstance->overrideSize = Len;
    // 复制覆盖处指令
    // 逆向分析已知是14字节
    HookExInstance->hookTargetRawCode = std::vector<uint8_t>(Target, Target + HookExInstance->overrideSize);

    // 为调度器汇编部分分配空间
    HookExInstance->pAsmDispatcher = VirtualAlloc(0, allocSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!HookExInstance->pAsmDispatcher) {
        MulNX::ErrorTerminate("windows内存分配失败！");
    }

    
    {
        // 创建编译器
        using enum MulNX::Memory::Asm::Reg;
        using namespace MulNX::Memory::Asm;
        Assembler Asm{};

        // 计算结构体大小（保证16字节对齐）
        const size_t ctxSize = (sizeof(RegContext) + 15) & ~15;

        // 分配栈空间
        Asm
            .sub(RSP, ctxSize);

        // 保存所有寄存器到 [rsp + offset]
        Asm
            .mov(Mem(RSP, offsetof(RegContext, rax)), RAX)
            .mov(Mem(RSP, offsetof(RegContext, rcx)), RCX)
            .mov(Mem(RSP, offsetof(RegContext, rdx)), RDX)
            .mov(Mem(RSP, offsetof(RegContext, rbx)), RBX)
            .mov(Mem(RSP, offsetof(RegContext, rsp)), RSP) // 保存当前rsp（分配后的）
            .mov(Mem(RSP, offsetof(RegContext, rbp)), RBP)
            .mov(Mem(RSP, offsetof(RegContext, rsi)), RSI)
            .mov(Mem(RSP, offsetof(RegContext, rdi)), RDI)
            .mov(Mem(RSP, offsetof(RegContext, r8)), R8)
            .mov(Mem(RSP, offsetof(RegContext, r9)), R9)
            .mov(Mem(RSP, offsetof(RegContext, r10)), R10)
            .mov(Mem(RSP, offsetof(RegContext, r11)), R11)
            .mov(Mem(RSP, offsetof(RegContext, r12)), R12)
            .mov(Mem(RSP, offsetof(RegContext, r13)), R13)
            .mov(Mem(RSP, offsetof(RegContext, r14)), R14)
            .mov(Mem(RSP, offsetof(RegContext, r15)), R15);


        // 指令执行区
        Asm
            .mov(RCX, (uintptr_t)HookExInstance.get())// 此参数对应 HookEx，是第一个参数
            .mov(RDX, RSP)// 此参数对应 RegContext，是第二个参数
            .mov(RAX, (uintptr_t)&MulNX::Memory::HookEx::Dispatch)// 绑定分发函数
            .sub(RSP, 32)// 分配影子空间
            .call(RAX)// 调用函数，进入CPP语言空间
            .add(RSP, 32);// 回收影子空间

        // 恢复区
        Asm
            .mov(RAX, Mem(RSP, offsetof(RegContext, rax)))
            .mov(RCX, Mem(RSP, offsetof(RegContext, rcx)))
            .mov(RDX, Mem(RSP, offsetof(RegContext, rdx)))
            .mov(RBX, Mem(RSP, offsetof(RegContext, rbx)))
            // 不恢复 rsp
            .mov(RBP, Mem(RSP, offsetof(RegContext, rbp)))
            .mov(RSI, Mem(RSP, offsetof(RegContext, rsi)))
            .mov(RDI, Mem(RSP, offsetof(RegContext, rdi)))
            .mov(R8, Mem(RSP, offsetof(RegContext, r8)))
            .mov(R9, Mem(RSP, offsetof(RegContext, r9)))
            .mov(R10, Mem(RSP, offsetof(RegContext, r10)))
            .mov(R11, Mem(RSP, offsetof(RegContext, r11)))
            .mov(R12, Mem(RSP, offsetof(RegContext, r12)))
            .mov(R13, Mem(RSP, offsetof(RegContext, r13)))
            .mov(R14, Mem(RSP, offsetof(RegContext, r14)))
            .mov(R15, Mem(RSP, offsetof(RegContext, r15)));

        // 释放上下文空间
        Asm
            .add(RSP, ctxSize);

        // 临时：给RAX分配一个合法内存，让CS2不崩溃
        Asm
            .mov(RAX, (uintptr_t)&MyAddable);

        HookExInstance->dispatcherAsmCode = Asm.Release();
    
        // 复制原始指令，等到集成反汇编引擎处理相对寻址
        //HookExInstance->CodeCaller.push_back(std::move(Raw));
    
        // 跳转到原处
        Assembler Asm2{};
        Asm2
            .jmp64((uintptr_t)Target + HookExInstance->overrideSize);
        
        HookExInstance->dispatcherAsmCode.push_back(std::move(Asm2.Release()));
    }
    // 复制机器码到VirtualAlloc分配的内存
    memcpy(HookExInstance->pAsmDispatcher, HookExInstance->dispatcherAsmCode.Data(), HookExInstance->dispatcherAsmCode.Size());

    {
        using enum MulNX::Memory::Asm::Reg;
        using namespace MulNX::Memory::Asm;
        Assembler Asm{};

        Asm.jmp64((uintptr_t)HookExInstance->pAsmDispatcher);
        for (int i = 0;i < HookExInstance->overrideSize;++i) {
            Asm.nop();
        }
        HookExInstance->jumperAsmCode = Asm.Release();
    }

    return HookExInstance;
}

MulNX::Memory::HookEx::Result MulNX::Memory::HookEx::Attach() {
    __try {
        if (this->attached)return MulNX::Memory::HookEx::Result::Attached;
        // 覆盖原位置
        DWORD old;
        VirtualProtect(this->hookTarget, this->overrideSize, PAGE_EXECUTE_READWRITE, &old);
        memcpy(this->hookTarget, this->jumperAsmCode.Data(), this->overrideSize);
        VirtualProtect(this->hookTarget, this->overrideSize, old, &old);

        this->attached = true;

        return MulNX::Memory::HookEx::Result::AttachSuccess;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return MulNX::Memory::HookEx::Result::AttachError;
    }
}

MulNX::Memory::HookEx::Result MulNX::Memory::HookEx::Detach() {
    __try {
        if (!this->attached)return MulNX::Memory::HookEx::Result::Detached;
        // 覆盖原位置
        DWORD old;
        VirtualProtect(this->hookTarget, this->overrideSize, PAGE_EXECUTE_READWRITE, &old);
        memcpy(this->hookTarget, this->hookTargetRawCode.data(), this->overrideSize);
        VirtualProtect(this->hookTarget, this->overrideSize, old, &old);

        this->attached = false;

        return MulNX::Memory::HookEx::Result::DetachSuccess;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return MulNX::Memory::HookEx::Result::DetachError;
    }
}

MulNX::Memory::HookEx::~HookEx() {
    if (this->pAsmDispatcher == nullptr)return;
    this->Detach();
    std::atomic_ref<size_t> threads(this->threadNumInAsm);
    while (threads.load(std::memory_order_acquire) > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    VirtualFree(this->pAsmDispatcher, 0, MEM_RELEASE);
}