#include "WinException.hpp"

#include <Windows.h>

bool MulNX::Base::UnsafeFunc(const std::function<bool()>&Func) {
    __try {
        return Func();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}