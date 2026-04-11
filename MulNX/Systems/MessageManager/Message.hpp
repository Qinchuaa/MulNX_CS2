#pragma once

#include <MulNX/Base/any_smart_ptr/any_smart_ptr.hpp>
#include <MulNX/Config/Config.hpp>
#include <cstdint>

namespace MulNX {

    template<typename T>
    concept Clike = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

    template<typename T>
    concept Clike4 = Clike<T> && (sizeof(T) == 4);

    template<typename T>
    concept Clike8 = Clike<T> && (sizeof(T) == 8);

    class Param {
        uint64_t internal;
    public:
        template<Clike8 T>
        T& as() {
            return reinterpret_cast<T&>(internal);
        }
        template<Clike4 T>
        T& low() {
            return reinterpret_cast<T&>(*reinterpret_cast<uint32_t*>(&internal));
        }
        template<Clike4 T>
        T& high() {
            return reinterpret_cast<T&>(*reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(&internal) + 4));
        }
    };
    static_assert(alignof(Param) == 8, "Param 未按 8 字节对齐");

    // MulNX消息
	class Message {
	public:
		// 消息类型，用于区分消息
		size_t type;
        Param p1;
        Param p2;
        // 8字节类型安全擦除共享指针，使用时需用get方法正确恢复，类型错误返回nullptr
        MulNX::any_shared_ptr asp = nullptr;

		Message() = default;
		Message(size_t Type) :type(Type) {}
        Message(const Message& Other) = default;
        Message& operator=(const Message& other) = default;
        Message(Message&& other) = default;
        Message& operator=(Message&& other) = default;

        template<typename T, typename... Args>
        static std::pair<Message,T*> Create(size_t type, Args&&... args) {
            Message msg(type);
            auto [p, rp] = MulNX::make_any_shared<T>(std::forward<Args>(args)...);
            msg.asp = std::move(p);
            return std::make_pair(std::move(msg), rp);
        }
    };
}
