#pragma once

#include <MulNX/Core/ModuleBase/ModuleBase.hpp>
#include <MulNX/Systems/MessageManager/Message.hpp>

#include <algorithm>
#include <deque>
#include <string>
#include <filesystem>
#include <fstream>

namespace MulNX {
    class Debugger final :public MulNX::ModuleBase {
    private:
        MulNX::Logger* pLogger = nullptr;
        std::string kInfo{};
        std::string kSucc{};
        std::string kWarning{};
        std::string kError{};
        std::deque<std::string> DebugMsg{};
    public:
		bool IfShowStream = true;
		int MaxMsgCount = 1000;
        bool ShowWhenError = true;
        bool AutoScroll = true;
		bool NeedAutoScroll = false;

		bool Init()override;
		void ProcessMsg(MulNX::Message& Msg)override;
	private:
        void Main();
        bool UINodeFunc(MulNX::UINode* ThisNode);		
        void ResetMaxMsgCount(const int Max);
        void PushBack(MulNX::NetExt&& pack,const std::string& strLevel);
    public:
		void ShowStream();
		void HideStream();
	};
}