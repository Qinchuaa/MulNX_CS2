#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>

namespace MulNX {
	class Debugger;
	class IDebugger :public MulNX::ModuleBase {
	public:
		virtual void AddInfo(const std::string& NewMsg) = 0;
		virtual void AddSucc(const std::string& NewMsg) = 0;
		virtual void AddWarning(const std::string& NewMsg) = 0;
		virtual void AddError(const std::string& NewMsg) = 0;
		virtual void SetShowFunc(std::function<void(Debugger*)>ShowFunc) = 0;

		bool ShowWhenError = true;
		bool AutoScroll = true;
	};
}
