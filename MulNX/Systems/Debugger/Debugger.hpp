#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>

#include <algorithm>
#include <deque>
#include <string>
#include <filesystem>
#include <fstream>

bool MySaveStringToFile(const std::string& data,
    const std::filesystem::path& filePath);

namespace MulNX {
    class Debugger final :public MulNX::ModuleBase {
    public:
		std::deque<std::string> DebugMsg{};
		bool IfShowStream = true;
		int MaxMsgCount = 1000;


		const std::string Info = "[提示]";
		const std::string Succ = "[成功]";
		const std::string Warning = "[警告]";
		const std::string Error = "[错误]";
	public:
        bool ShowWhenError = true;
        bool AutoScroll = true;
		bool NeedAutoScroll = false;

		//模块基类接口实现

		bool Init()override;
		void Main();
		void ProcessMsg(MulNX::Message& Msg)override;
		bool UINodeFunc(MulNX::UINode* ThisNode);		
	private:
        void AddInfo(const std::string& NewMsg);
        void AddSucc(const std::string& NewMsg);
        void AddWarning(const std::string& NewMsg);
        void AddError(const std::string& NewMsg);
        void ResetMaxMsgCount(const int Max);
        void SaveToFile();
    public:
		//其它函数

		void PushBack(const std::string& NewMsg, const std::string& prefix);

		void ShowStream();
		void HideStream();
	};
}