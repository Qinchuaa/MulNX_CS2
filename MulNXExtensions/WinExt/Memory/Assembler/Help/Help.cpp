#include "../Assembler.hpp"

// 根据位移量确定 mod 字段和位移大小（返回 mod 值，并通过 bytes 输出位移字节数）
int get_mod_and_disp(int64_t disp, int& bytes) {
    if (disp == 0) {
        bytes = 0;
        return 0; // mod=00，无位移
    }
    else if (disp >= -128 && disp <= 127) {
        bytes = 1;
        return 1; // mod=01，8位位移
    }
    else {
        bytes = 4;
        return 2; // mod=10，32位位移
    }
}