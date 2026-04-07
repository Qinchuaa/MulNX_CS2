#include "Hook.hpp"

#include <Windows.h>
#include <atomic>
#include <algorithm>
#include <format>

uintptr_t MulNX::Memory::HookEx::Dispatch(HookEx* pHookExInstance, RegContext* ctx) {
    // 返回true继续执行剩余指令，返回false则在恢复寄存器后直接ret
    if (pHookExInstance->callback(ctx, pHookExInstance)) {
        return pHookExInstance->jmpTarget1;
    }
    else {
        return pHookExInstance->jmpTarget0;
    }
}

void* TryAlloc(uintptr_t target, size_t size) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    // 将 DWORD 转换为 uintptr_t 以避免位扩展警告
    uintptr_t allocGran = static_cast<uintptr_t>(si.dwAllocationGranularity); // 通常 64KB
    uintptr_t pageSize = static_cast<uintptr_t>(si.dwPageSize);              // 通常 4KB

    // 将 size 向上对齐到页面大小
    size = (size + pageSize - 1) & ~(pageSize - 1);

    // 搜索范围：target ± 1GB
    const uintptr_t range = 1024ULL * 1024 * 1024;  // 1GB
    uintptr_t startAddr = (target > range) ? (target - range) : 0x10000; // 避免低 64KB
    uintptr_t endAddr = target + range;

#ifdef _WIN64
    const uintptr_t maxUser = 0x7FFFFFFF0000ULL; // 64 位用户空间上限示例
#else
    const uintptr_t maxUser = 0x7FFEFFFF;        // 32 位用户空间典型上限
#endif
    if (endAddr > maxUser) endAddr = maxUser;

    // 步长取分配粒度与 1MB 中较大者
    uintptr_t step = std::max(allocGran, static_cast<uintptr_t>(1024 * 1024));

    // 1. 尝试直接在 target 处分配
    LPVOID p = VirtualAlloc(reinterpret_cast<LPVOID>(target), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (p) return p;

    // 2. 从 target 向低地址和高地址交替尝试
    uintptr_t low = target;
    uintptr_t high = target;

    while (low > startAddr || high < endAddr) {
        // 尝试低地址
        if (low > startAddr) {
            low = (low > step) ? low - step : startAddr;
            // 对齐到分配粒度：注意使用 uintptr_t 类型的掩码
            low = (low + allocGran - 1) & ~(allocGran - 1);
            p = VirtualAlloc(reinterpret_cast<LPVOID>(low), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (p) return p;
        }

        // 尝试高地址
        if (high < endAddr) {
            high = (high < endAddr - step) ? high + step : endAddr;
            high = high & ~(allocGran - 1);  // 向下对齐
            p = VirtualAlloc(reinterpret_cast<LPVOID>(high), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (p) return p;
        }
    }

    return nullptr;
}

std::expected<std::unique_ptr<MulNX::Memory::HookEx>, std::string> MulNX::Memory::HookEx::Create(uint8_t* Target, int Len, bool extraStackAdjust, std::function<bool(RegContext*, HookEx*)>&& callback) {
    if (0 < Len && Len < 5) {
        return std::unexpected(std::format("参数指定的长度是：{}  ，这个长度怎么可能放得下一个jmp rel32？？", Len));
    }
    if (Len == 0) {
        // 自动分析法，info至少提供20个空间
        auto info = MulNX::Memory::HookEx::AnalyseTarget(Target);
        for (int i = 0;Len < 5;++i) {
            Len += info.Cmds.at(i).size;
        }
    }

    // 创建HookEx实例
    auto HookExInstance = std::make_unique<HookEx>();
    HookExInstance->hookTarget = Target;
    HookExInstance->callback = std::move(callback);
    HookExInstance->overrideSize = Len;
    // 复制覆盖处指令
    HookExInstance->hookTargetRawCode = MulNX::Memory::Asm::Code(Target, Target + HookExInstance->overrideSize);

    // 为调度器汇编部分分配空间    
    auto* alloced = TryAlloc((uintptr_t)Target, 4096);
    if (!alloced) {
        return std::unexpected("windows内存分配失败！无法找到空间");
    }
    // 进行raii绑定，防止内存泄露
    HookExInstance->pAsmDispatcher = alloced;
    if (std::abs(static_cast<long long>(reinterpret_cast<uintptr_t>(alloced) -
        reinterpret_cast<uintptr_t>(Target))) > 1024ULL * 1024 * 1024) {
        return std::unexpected("windows内存分配失败！分配空间不合适");
    }
    
    {
        // 创建编译器
        using enum MulNX::Memory::Asm::Reg;
        using namespace MulNX::Memory::Asm;
        Assembler Asm{};

        // 计算结构体大小（保证16字节对齐）
        constexpr size_t ctxSize = (sizeof(RegContext) + 15) & ~15;
        size_t frameSize = ctxSize;
        if (!extraStackAdjust)frameSize += 8;

        Asm
            .sub(RSP, frameSize) // 分配栈空间
            // 保存所有寄存器到 [rsp + offset]
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
            .add(RSP, 32)// 回收影子空间
            .jmp(RAX);// 某种意义上，这里是一个分支操作，但是这个分支实际上是由CPP语言层面决定的，因此我们在汇编层面上并不需要区分真假分支，直接让它无条件跳转到jmpTarget即可
        HookExInstance->dispatcherAsmCode = Asm.Release();

        // 分支0
        HookExInstance->jmpTarget0 = (uintptr_t)HookExInstance->pAsmDispatcher + HookExInstance->dispatcherAsmCode.size();
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
            .mov(R15, Mem(RSP, offsetof(RegContext, r15)))
            // 释放上下文空间
            .add(RSP, frameSize)
            .ret();// 从分发函数返回

        HookExInstance->dispatcherAsmCode.append_range(std::move(Asm.Release()));







        // 分支1
        HookExInstance->jmpTarget1 = (uintptr_t)HookExInstance->pAsmDispatcher + HookExInstance->dispatcherAsmCode.size();
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
            .mov(R15, Mem(RSP, offsetof(RegContext, r15)))
            // 释放上下文空间
            .add(RSP, frameSize);

        HookExInstance->dispatcherAsmCode.append_range(std::move(Asm.Release()));

        // 这里恰好可以记录一个可能是原函数地址的位置（如果覆盖的指令是一个完整函数的开头），供回调函数使用
        HookExInstance->pMaybeRawFunc = (uintptr_t)HookExInstance->pAsmDispatcher + HookExInstance->dispatcherAsmCode.size();

        // 修复原始指令
        auto result = FixRIPRelativeInstructions(HookExInstance->hookTargetRawCode,
            (uintptr_t)HookExInstance->hookTarget,
            (uintptr_t)HookExInstance->pAsmDispatcher + HookExInstance->dispatcherAsmCode.size());
        if (!result.has_value())return std::unexpected(result.error());

        MulNX::Memory::Asm::Code fixed = result.value();
            

        // 追加原始指令
        HookExInstance->dispatcherAsmCode.append_range(std::move(fixed));

        // 跳转到原处
        Asm.jmp64((uintptr_t)Target + HookExInstance->overrideSize);
        
        HookExInstance->dispatcherAsmCode.append_range(std::move(Asm.Release()));













        // 复制机器码到VirtualAlloc分配的内存
        memcpy(HookExInstance->pAsmDispatcher,
            HookExInstance->dispatcherAsmCode.data(),
            HookExInstance->dispatcherAsmCode.size());



        


        







        
        // 生成用于覆盖原位置的代码
        Asm.jmp((uintptr_t)HookExInstance->pAsmDispatcher - (uintptr_t)Target - 5);
        //Asm.jmp64((uintptr_t)HookExInstance->pAsmDispatcher);
        for (int i = 5;i < HookExInstance->overrideSize;++i) {
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
        memcpy(this->hookTarget, this->jumperAsmCode.data(), this->overrideSize);
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