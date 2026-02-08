#pragma once

#include<filesystem>

class MulNXeConfig {
public:
	std::filesystem::path CS2Path;
};


class MulNXeCore;

class ConfigManager {
private:
	MulNXeConfig Config{};
	MulNXeCore* Core;
public:
	bool Init(MulNXeCore* Core);

	bool Config_SetCS2Path(const std::filesystem::path& CS2Path);
	

	bool Config_Save(const std::filesystem::path& ConfigPath)const;

	bool Config_Load(const std::filesystem::path& ConfigPath);

	std::filesystem::path Config_GetCS2Path()const;

	
};