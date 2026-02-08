#pragma once

#include"../../Core/ModuleBase/ModuleBase.hpp"

#include<filesystem>
#include<string>
#include<Windows.h>

class Paths {
public:
    //std::filesystem::path GetPath();
    // 主路径结构
    struct S_MulNX {
        std::filesystem::path Path;

        struct S_CS2Resources {
            std::filesystem::path Path;       // MulNXConfig文件
            struct S_Cfg {
                std::filesystem::path Path;
			} Cfg;
        } CS2Resources;

        struct S_DLLToCS {
            std::filesystem::path Path;       // DLLToCS工具目录
            struct S_MulNXDLL_dll {
                std::filesystem::path Path;
            }MulNXDLL_dll;
        }DLLToCS;

        struct S_Saves {
            std::filesystem::path Path;       // Saves主目录
            struct S_Internal {
                std::filesystem::path Path;
                struct S_Core {
                    std::filesystem::path Path;
                    struct S_CameraSystem {
                        std::filesystem::path Path;
                        struct S_Workspaces {
                            std::filesystem::path Path;
                            struct S_Workspace {
                                std::filesystem::path Path;
                                struct S_Project {
                                    std::filesystem::path Path;
                                    struct S_Elements {
                                        std::filesystem::path Path;   // Elements子目录
                                    } Elements;
                                    struct S_Solutions {
                                        std::filesystem::path Path;   // Solutions子目录
                                    } Solutions;
                                } Project;
                            } Workspace;
                        } Workspaces;
                    } CameraSystem;
                } Core;
            } Internal;
        } Saves;

        struct S_MulNX_exe {
            std::filesystem::path Path;
        } MulNX_exe;

    } MulNX;

    struct S_Counter_Strike_Global_Offensive {
        std::filesystem::path Path;

        struct S_game {
            std::filesystem::path Path;

            struct S_bin {
                std::filesystem::path Path;

                struct S_win64 {
                    std::filesystem::path Path;

                    struct S_cs2_exe {
                        std::filesystem::path Path;
                    }cs2_exe;
                } win64;
			} bin;

            struct S_csgo {
                std::filesystem::path Path;

                struct S_cfg {
                    std::filesystem::path Path;
				} cfg;
			} csgo;
        }game;
	} Counter_Strike_Global_Offensive;
};

namespace MulNX {
    class IPCer final :public MulNX::ModuleBase {
        Paths Paths{};
    public:
        IPCer() : ModuleBase() {
            //this->Type = MulNX::ModuleType::IPCer;
        }

        bool Inited = false;

        bool Init()override;

        bool GetWindowPathByName(const LPCWSTR& WindowName, std::filesystem::path& Output);

        std::filesystem::path PathGet_Workspaces();

        std::filesystem::path PathGet_CurrentWorkspace();
        std::filesystem::path PathGet_CurrentProject();
        std::filesystem::path PathGet_CurrentSolutions();
        std::filesystem::path PathGet_CurrentElements();

        std::filesystem::path PathGet_MulNX();
        std::filesystem::path PathGet_CS2Resources();
        std::filesystem::path PathGet_Tool_Cfg();

        std::filesystem::path PathGet_DLLToCS();
        std::filesystem::path PathGet_MulNXDLL_dll();
        std::filesystem::path PathGet_Saves();
        std::filesystem::path PathGet_Internal();
        std::filesystem::path PathGet_Core();
        std::filesystem::path PathGet_CameraSystem();
        std::filesystem::path PathGet_MulNX_exe();

        std::filesystem::path PathGet_CS_cfg();

        bool PathCreate_Workspace(const std::string& NewWorkspaceName);
        bool PathSet_Workspace(const std::string& NewWorkspaceName);

        bool PathCreate_Project(const std::string& NewProjectName);
        bool PathSet_Project(const std::string& NewProjectName);

        std::string GetAllPathMsg();

        std::vector<std::string> GetProjectsNames();
        std::vector<std::string> GetFileNamesByPath(std::filesystem::path& FolderPath);

        bool GetFileNames(std::vector<std::string>& FileNames, const std::filesystem::path& FolderPath, const std::vector<std::string>& Filter, const bool Extension = false);
        bool FileDelete(const std::string& FileName, const std::filesystem::path& FolderPath);
        bool FileMove(const std::string& FileName, const std::filesystem::path& Resource, const std::filesystem::path& Target);
    };
}