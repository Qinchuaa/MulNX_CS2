#pragma once

#include <MulNX/Base/MulNXHandle/MulNXHandle.hpp>
#include <MulNX/Config/Config.hpp>

namespace MulNX {
	class IMessageChannel {
	public:
        virtual IMessageChannel& Subscribe(const std::string& Type) = 0;
		virtual bool PushMessage(Message&& InMsg) = 0;
		virtual bool PullMessage(Message& OutMsg) = 0;

		virtual bool HasMessage()const = 0;

		virtual MulNXHandle GetHandle()const = 0;

		virtual ~IMessageChannel() = default;
	};
}