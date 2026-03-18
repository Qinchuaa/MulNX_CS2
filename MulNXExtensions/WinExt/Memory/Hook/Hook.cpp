#include "Hook.hpp"

#include <MulNX/Config/Config.hpp>
#include <MulNXExtensions/WinExt/WinException/WinException.hpp>
#include <Zydis/Zydis.h>
#include <Windows.h>
#include <atomic>
#include <algorithm>

// 修复原始指令中的RIP相对寻址
// raw_code: 原始机器码
// old_base: 原始代码地址（hookTarget）
// new_base: 调度器中的新地址（pAsmDispatcher + 已生成代码大小）
std::vector<uint8_t> FixRIPRelativeInstructions(
    const std::vector<uint8_t>& raw_code,
    uintptr_t old_base,
    uintptr_t new_base) {
    std::vector<uint8_t> fixed;
    ZydisDecoder decoder;
    ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64);

    size_t offset = 0;
    while (offset < raw_code.size()) {
        ZydisDecodedInstruction instr;
        ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];
        ZyanStatus status = ZydisDecoderDecodeFull(&decoder,
            raw_code.data() + offset,
            raw_code.size() - offset,
            &instr, operands);

        if (!ZYAN_SUCCESS(status)) {
            // 解码失败，直接复制剩余字节（理论上不应发生，但做保护）
            fixed.insert(fixed.end(), raw_code.begin() + offset, raw_code.end());
            break;
        }

        uintptr_t old_instr_addr = old_base + offset;
        uintptr_t new_instr_addr = new_base + fixed.size();

        // 提取当前指令原始字节（将用于修改）
        std::vector<uint8_t> instr_bytes(
            raw_code.begin() + offset,
            raw_code.begin() + offset + instr.length);

        // 遍历操作数，查找RIP相对寻址
        for (size_t i = 0; i < instr.operand_count; ++i) {
            const auto& op = operands[i];
            if (op.type == ZYDIS_OPERAND_TYPE_MEMORY &&
                op.mem.base == ZYDIS_REGISTER_RIP) {
                // 获取原始位移（注意可能是负值）
                // Zydis stores displacement info in `disp.size`/`disp.value` (no `has_displacement` flag)
                int64_t orig_disp = (op.mem.disp.size != 0) ? op.mem.disp.value : 0;

                // 原始目标地址 = 旧指令地址 + 指令长度 + 原始位移
                uint64_t target = old_instr_addr + instr.length + orig_disp;

                // 新位移 = 目标 - (新指令地址 + 指令长度)
                int64_t new_disp = target - (new_instr_addr + instr.length);

                // 检查32位范围（RIP相对寻址使用有符号32位）
                if (new_disp < -2147483648LL || new_disp > 2147483647LL) {
                    // 超出范围，需要更复杂的处理（比如蹦床），此处可记录错误或断言
                    // 暂时仍赋值，但可能导致崩溃
                    MulNX::ErrorTerminate("无法成功！");
                }

                // 定位指令中的位移字段并修改
                // `instr.raw.disp.size` is zero when no displacement bytes are present
                if (instr.raw.disp.size != 0) {
                    size_t disp_offset = instr.raw.disp.offset;               // 字节偏移
                    size_t disp_size = instr.raw.disp.size / 8;             // 字节数（通常为4）
                    int32_t disp_to_write = static_cast<int32_t>(new_disp);
                    memcpy(instr_bytes.data() + disp_offset, &disp_to_write, disp_size);
                }
                break;  // 一条指令一般只有一个RIP相对操作数
            }
        }

        // 将修复后的指令加入结果
        fixed.insert(fixed.end(), instr_bytes.begin(), instr_bytes.end());
        offset += instr.length;
    }
    return fixed;
}
void MulNX::Memory::HookEx::Dispatch(HookEx* pHookExInstance, RegContext* ctx) {
    pHookExInstance->callback(ctx);
    return;
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

std::unique_ptr<MulNX::Memory::HookEx> MulNX::Memory::HookEx::Create(uint8_t* Target, int Len, std::function<void(RegContext*)>&& callback) {
    // 首先创建HookEx实例
    auto HookExInstance = std::make_unique<HookEx>();
    HookExInstance->hookTarget = Target;
    HookExInstance->callback = std::move(callback);
    HookExInstance->overrideSize = Len;
    // 复制覆盖处指令
    HookExInstance->hookTargetRawCode = std::vector<uint8_t>(Target, Target + HookExInstance->overrideSize);

    // 为调度器汇编部分分配空间    
    auto* alloced = TryAlloc((uintptr_t)Target, 4096);
    if (!alloced) {
        MulNX::ErrorTerminate("windows内存分配失败！");
    }
    if (std::abs(static_cast<long long>(reinterpret_cast<uintptr_t>(alloced) -
        reinterpret_cast<uintptr_t>(Target))) > 1024ULL * 1024 * 1024) {
        MulNX::ErrorTerminate("windows内存分配失败！");
    }
    HookExInstance->pAsmDispatcher = alloced;

    {
        // 创建编译器
        using enum MulNX::Memory::Asm::Reg;
        using namespace MulNX::Memory::Asm;
        Assembler Asm{};

        // 计算结构体大小（保证16字节对齐）
        const size_t ctxSize = (sizeof(RegContext) + 15) & ~15;

        Asm
            .sub(RSP, ctxSize) // 分配栈空间
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
            .mov(R15, Mem(RSP, offsetof(RegContext, r15)))
            // 释放上下文空间
            .add(RSP, ctxSize);

        HookExInstance->dispatcherAsmCode = Asm.Release();
    
        // 修复原始指令
        auto fixed =
            FixRIPRelativeInstructions(HookExInstance->hookTargetRawCode,
                (uintptr_t)HookExInstance->hookTarget,
                (uintptr_t)HookExInstance->pAsmDispatcher + HookExInstance->dispatcherAsmCode.size());

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
        Asm.jmp64((uintptr_t)HookExInstance->pAsmDispatcher);
        for (int i = 14;i < HookExInstance->overrideSize;++i) {
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