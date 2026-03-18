#pragma once

#include <functional>
#include <Windows.h>
namespace MulNX {
    namespace Base {
        // 安全的调用包装器：捕获结构化异常，返回 bool 表示成功
        template<typename F, typename... Args>
        bool SEHCall(F&& f, Args&&... args) {
            __try {
                std::forward<F>(f)(std::forward<Args>(args)...);
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return false;
            }
        }
	}
}