#pragma once

typedef unsigned int UINT;

namespace MulNX {
	namespace Base {
		namespace WIN32Msg {
			//判断消息是否是鼠标消息
			bool IsMouseMessage(UINT uMsg);
			//判断消息是否是键盘消息
			bool IsKeyboardMessage(UINT uMsg);
		}
	}
}