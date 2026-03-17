#pragma once

#include <MulNX/Base/any_smart_ptr/any_smart_ptr.hpp>
#include <MulNX/Config/Config.hpp>

namespace MulNX {
	// MulNX消息基类
	class Message {
	public:
		// 消息类型，用于区分消息
		size_t type;
        union { int i;float f; }p1;
        union { int i;float f; }p2;
        union { int i;float f; }p3;
        union { int i;float f; }p4;
        // 8字节类型安全擦除共享指针，使用时需用get方法正确恢复，类型错误返回nullptr
        MulNX::any_shared_ptr asp = nullptr;
        // 消息管道指针，一般用于指示消息来源

		Message() = default;
		Message(size_t Type) :type(Type) {}
        Message(const Message& Other) = default;
        Message& operator=(const Message& other) = default;
        Message(Message&& other) = default;
        Message& operator=(Message&& other) = default;

        void Clear();

        template<typename T, typename... Args>
        static Message Create(size_t type, Args&&... args) {
            Message msg(type);
            auto [p, rp] = MulNX::make_any_shared<T>(std::forward<Args>(args)...);
            msg.asp = std::move(p);
            return msg;
        }
    };
}
