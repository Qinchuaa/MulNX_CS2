#pragma once

#include "../../Core/ModuleBase/ModuleBase.hpp"

#include "MulNXMessage/MulNXMessage.hpp"
#include "MessageChannel/IMessageChannel.hpp"

namespace MulNX {
    // FNV-1a 64位常数
    constexpr std::size_t fnv_basis = 14695981039346656037ULL;
    constexpr std::size_t fnv_prime = 1099511628211ULL;
    // FNV-1a 核心算法（constexpr）
    constexpr std::size_t HashString(const std::string& str) noexcept {
        std::size_t hash = fnv_basis;
        for (unsigned char c : str) {
            hash = (hash ^ static_cast<std::size_t>(c)) * fnv_prime;
        }
        return hash;
    }
}

// 定义字面量操作符
consteval size_t operator"" _hash(const char* str, size_t n) {
    std::string Str(str, n);
    return MulNX::HashString(Str);
}

namespace MulNX {
	class IMessageManager :public ModuleBase {
			friend IMessageChannel;
			friend class MessageChannel;
		public:
			// 创建私有消息队列（但是生命周期仍然委托给消息管理器）
			virtual MulNXHandle CreateMessageChannel() = 0;
			// 获取消息管道
			virtual IMessageChannel* GetMessageChannel(const MulNXHandle& hChannel) = 0;
			// 发布，在堆空间创建消息后传递，所有权转移至总线
			virtual bool Publish(Message&& Msg) = 0;
    };
}