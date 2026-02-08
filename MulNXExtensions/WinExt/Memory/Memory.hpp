#pragma once

#include <atomic>
#include <string>
#include <Windows.h>

namespace MulNX {
	namespace Base {
		namespace Memory {
			//安全访问，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
			template<typename T>
			inline bool Read(const uintptr_t Address, T& Value) {
				__try {
					Value = *reinterpret_cast<T*>(Address);
					return true;
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					return false;
				}
			}
			//安全访问，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
			template<typename T>
			inline bool Read(const T* Address, T& Value) {
				__try {
					Value = *Address;
					return true;
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					return false;
				}
			}
			//安全访问，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
			template<typename T>
			inline bool Read(const uintptr_t Address, std::atomic<T>& Value) {
				__try {
					Value.store(*reinterpret_cast<T*>(Address));
					return true;
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					return false;
				}
			}
			//安全访问，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
			template<typename T>
			inline T Read(const uintptr_t Address) {
				__try {
					return *reinterpret_cast<T*>(Address);
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					return T{};
				}
			}
			//安全写入，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
			template<typename T>
			inline bool Write(T* Address, const T& Value) {
				__try {
					*Address = Value;
					return true;
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					return false;
				}
			}
			//安全写入，避免访问冲突，可能在关闭游戏时报错，正常，因为本程序本身被中断了，来不及执行后面的内容
			template<typename T>
			inline bool Write(const uintptr_t Address, const T& Value) {
				__try {
					*reinterpret_cast<T*>(Address) = Value;
					return true;
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					return false;
				}
			}

			//安全读取字符串（ANSI/UTF-8），逐字节读取直到遇到空字符或达到缓冲大小
			bool ReadString(const uintptr_t Address, char* Buffer, size_t BufferSize);
			//安全读取宽字符串（UTF-16），逐字符读取直到遇到空字符或达到缓冲字符数
			bool ReadWString(const uintptr_t Address, wchar_t* Buffer, size_t BufferCount);
		}
	}
}