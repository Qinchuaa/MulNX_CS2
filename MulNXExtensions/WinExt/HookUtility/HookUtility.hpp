#pragma once

#include <MulNXThirdParty/All_MinHook.hpp>

#include <memory>
#include <functional>

namespace MulNX {
    namespace Base {
        namespace HookUtility {
            //Hook封装
            template<typename Func>
            class HookUtility;
            //主模板，注意一个函数类型一个实例（应该也只需要一个实例）
            template<typename Result, typename... Args>
            class HookUtility<Result(Args...)> {
            private:
                using FunctionType = Result(Args...);
                //目标函数指针
                FunctionType* pTargetFunc = nullptr;
                //原函数指针
                FunctionType* pOriginFunc = nullptr;
                //用户函数包装器
                std::function<Result(Args...)>MyFunc;
                //静态入口函数
                static Result EntryMyFunc(Args... args) {
                    //获取本实例化引用
                    auto& ThisInstance = HookUtility<Result(Args...)>::GetInstance();
                    Result ThisResult{};
                    //调用用户函数
                    if (ThisInstance.MyFunc) {
                        ThisResult = ThisInstance.MyFunc(args...);
                    }
                    //调用原始函数
                    if (ThisInstance.pOriginFunc) {
                        ThisResult = ThisInstance.pOriginFunc(args...);
                    }
                    //返回最后结果
                    return ThisResult;
                }
                //创建和启用标志
                bool Created = false;
                bool Enabled = false;
                //私有构造函数
                HookUtility() = default;
            public:
                //单例
                static HookUtility& GetInstance() {
                    static HookUtility Instance;
                    return Instance;
                }
                //设置目标函数
                void SetTarget(void* Target) {
                    this->pTargetFunc = (FunctionType*)Target;
                }
                //设置用户函数
                template<typename F>
                void SetMyFunction(F&& func) {
                    this->MyFunc = std::forward<F>(func);
                }
                //创建钩子
                bool Create() {
                    if (!this->pTargetFunc)return false;
                    if (MH_CreateHook(reinterpret_cast<void*>(this->pTargetFunc), reinterpret_cast<void*>(&this->EntryMyFunc),
                        reinterpret_cast<void**>(&this->pOriginFunc)) != MH_OK)return false;
                    this->Created = true;
                    return true;
                }
                //启用钩子
                bool Enable() {
                    if (!this->Created)return false;
                    if (MH_EnableHook(reinterpret_cast<void*>(pTargetFunc)) != MH_OK)return false;
                    this->Enabled = true;
                    return true;
                }
                //创建并启用钩子
                bool CreateAndEnable() {
                    if (!this->Create())return false;
                    if (!this->Enable())return false;
                    return true;
                }
                //禁用钩子
                bool Disable() {
                    if (!this->Enabled)return false;
                    if (MH_DisableHook(reinterpret_cast<void*>(this->pTargetFunc)) != MH_OK)return false;
                    this->Enabled = false;
                    return true;
                }
                //移除钩子
                bool Remove() {
                    if (!this->Created)return false;
                    if (MH_RemoveHook(reinterpret_cast<void*>(this->pTargetFunc)) != MH_OK)return false;
                    this->Created = false;
                    this->pOriginFunc = nullptr;
                    return true;
                }
                //清理所有资源
                void Clear() {
                    this->Disable();
                    this->Remove();
                    this->pTargetFunc = nullptr;
                    this->MyFunc = nullptr;
                }
                //析构函数
                ~HookUtility() {
                    this->Clear();
                }
            };
        }
    }
}