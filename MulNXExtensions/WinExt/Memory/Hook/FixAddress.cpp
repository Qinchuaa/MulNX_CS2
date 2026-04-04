#include "Hook.hpp"

#include <Zydis/Zydis.h>

// 修复原始指令中的RIP相对寻址
// raw_code: 原始机器码
// old_base: 原始代码地址（hookTarget）
// new_base: 调度器中的新地址（pAsmDispatcher + 已生成代码大小）
std::expected<std::vector<uint8_t>, std::string> MulNX::Memory::HookEx::FixRIPRelativeInstructions(
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
                    return std::unexpected("无法成功！");
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