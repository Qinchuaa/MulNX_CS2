#pragma once

#include <functional>

namespace MulNX {
	namespace Base {
		bool UnsafeFunc(const std::function<bool()>&Func);
	}
}