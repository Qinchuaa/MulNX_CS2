#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <MulNX/Systems/MessageManager/Message.hpp>

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


        std::string kInfo{};
        std::string kSucc{};
        std::string kWarning{};
        std::string kError{};
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
        void ResetMaxMsgCount(const int Max);
        void SaveToFile();
    public:
		//其它函数

		void PushBack(MulNX::NetExt&& pack,const std::string& strLevel);

		void ShowStream();
		void HideStream();
	};
}