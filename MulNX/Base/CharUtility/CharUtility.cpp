#include "CharUtility.hpp"

std::string MulNX::Base::CharUtility::FilePathToString(const std::filesystem::path& path) {
	std::u8string u8path = path.u8string();
	std::string utf8Path(u8path.begin(), u8path.end());
	std::replace(utf8Path.begin(), utf8Path.end(), '\\', '/');
	return utf8Path;
}