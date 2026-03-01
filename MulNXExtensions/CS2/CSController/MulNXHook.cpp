#include "CSController.hpp"

#include "../Signatures.hpp"

#include <MulNX/MulNX.hpp>
#include <MulNXExtensions/CameraSystem/CameraSystemIO/CameraSystemIO.hpp>
#include <MulNXThirdParty/All_cs2_dumper.hpp>
#include <MulNXThirdParty/All_ImGui.hpp>

//#include <cstdio>

int addable = 0;

struct RegContext {
    uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
};

using HookCallBack = void(*)(RegContext*);

void myCallBack(RegContext* RegCtx) {
    CSController::HandleOverrideView(reinterpret_cast<void*>(RegCtx->rsi));
    // MessageBoxA(nullptr, "buffer", "MulNX Dispatch", MB_OK);
}

struct HookInfo {
    int var1;
    int var2;
    HookCallBack callback;
};

void MulNX_Dispatch(const HookInfo* pInfo, RegContext* RegCtx) {
    // char buffer[256];
    //sprintf_s(buffer, "var1: %d, var2: %d", pInfo->var1, pInfo->var2);
    pInfo->callback(RegCtx);
}

// 全局或类静态成员
std::atomic<uintptr_t> atoPos = 0;
std::atomic<uintptr_t> atoCaller = 0;

void CSController::tempfunc(MulNX::Memory::Region& Target) {
    auto Guard = Target.ExchangeProtection(PAGE_EXECUTE_READWRITE);
    auto HookPos = Target.Data();

    // 复制覆盖处指令
    // 逆向分析已知是14字节
    std::vector<uint8_t> Raw(Target.Data(), Target.Data() + 14);

    // 为调度器汇编部分分配代码
    auto* pCaller = VirtualAlloc(0, 1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (pCaller) {
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
            auto* p_instance_HookInfo = reinterpret_cast<HookInfo*>(malloc(sizeof(HookInfo)));
            p_instance_HookInfo->var1 = 42;
            p_instance_HookInfo->var2 = 666;
            p_instance_HookInfo->callback = &myCallBack;
            Asm
                .mov(RCX, (uintptr_t)p_instance_HookInfo)
                .mov(RDX, RSP)
                .mov(RAX, (uintptr_t)&MulNX_Dispatch)
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


        // 临时：给RAX分配一个合法内存，让游戏不崩溃
        Asm
            .mov(RAX, (uintptr_t)&addable);
        
        CodeCaller = Asm.Release();
    }


    // 复制原始指令
    //CodeCaller.push_back(std::move(Raw));

    // 跳转到原处
    {
        using enum MulNX::Memory::Asm::Reg;
        using namespace MulNX::Memory::Asm;
        Assembler Asm{};
        Asm.nop().nop()
            .jmp64((uintptr_t)HookPos + 14)
            .nop()
            .nop()
            .nop();
        CodeCaller.push_back(std::move(Asm.Release()));
    }
    // 复制机器码到VirtualAlloc分配的内存
    memcpy(pCaller, CodeCaller.Data(), CodeCaller.Size());
    atoCaller = reinterpret_cast<uintptr_t>(pCaller);
















    // 修改原指令位置
    MulNX::Memory::Asm::Code Code{};
    {
        using enum MulNX::Memory::Asm::Reg;
        using namespace MulNX::Memory::Asm;
        Assembler Asm{};
        
        Asm.jmp64((uintptr_t)pCaller);
        Code = Asm.Release();
    }

    // 覆盖原位置
    memcpy(HookPos, Code.Data(), Code.Size());
    atoPos = (uintptr_t)HookPos;
}

bool CSController::UINodeFunc(MulNXUINode* ThisNode) {
    ImGui::Begin("测试222");

    // 从原子变量读取地址
    uintptr_t posVal = atoPos.load();
    uintptr_t CallerVal = atoCaller.load();

    // 用于显示的临时缓冲区
    char buf[64];

    snprintf(buf, sizeof(buf), "0x%p", reinterpret_cast<void*>(posVal));
    ImGui::InputText("目标地址", buf, sizeof(buf), ImGuiInputTextFlags_ReadOnly);
    // 汇编调度器部分地址
    snprintf(buf, sizeof(buf), "0x%p", reinterpret_cast<void*>(CallerVal));
    ImGui::InputText("调度器位置：", buf, sizeof(buf));

    ImGui::End();
    return true; // 根据实际需要返回值
}