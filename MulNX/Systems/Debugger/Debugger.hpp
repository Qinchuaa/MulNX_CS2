#pragma once

#include "IDebugger.hpp"

#include <algorithm>
#include <deque>
#include <string>
#include <filesystem>
#include <fstream>

bool MySaveStringToFile(const std::string& data,
    const std::filesystem::path& filePath);
namespace MulNX {
    class Debugger final :public IDebugger {
    public:
		std::deque<std::string> DebugMsg{};
		bool IfShowStream = true;
		int MaxMsgCount = 1000;


		const std::string Info = "[提示]";
		const std::string Succ = "[成功]";
		const std::string Warning = "[警告]";
		const std::string Error = "[错误]";

		std::function<void(Debugger*)> ShowFunc = nullptr;
	public:
		
		bool NeedAutoScroll = false;

		//模块基类接口实现

		bool Init()override;
		void VirtualMain()override;
		void ProcessMsg(MulNX::Message* Msg)override;
		bool UINodeFunc(MulNXUINode* ThisNode);

		//调试器接口实现

		void AddInfo(const std::string& NewMsg)override;
		void AddSucc(const std::string& NewMsg)override;
		void AddWarning(const std::string& NewMsg)override;
		void AddError(const std::string& NewMsg)override;

		void SetShowFunc(std::function<void(Debugger*)>Func)override {
			this->ShowFunc = Func;
		}
	private:
        void ResetMaxMsgCount(const int Max);
        void SaveToFile();
    public:
		//其它函数

		void PushBack(const std::string& NewMsg, const std::string& prefix);

		void ShowStream();
		void HideStream();
	};
}