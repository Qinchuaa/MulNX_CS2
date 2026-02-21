#include "Pattern.hpp"

#include <stdexcept>

MulNX::Memory::Pattern::Pattern(std::string&& Raw) {
    this->Raw = std::move(Raw);
    this->Bytes.clear();
    for (const char* CurrentChar = this->Raw.c_str(); CurrentChar < this->Raw.c_str() + this->Raw.size();) {
        if (*CurrentChar == ' ') {
            ++CurrentChar;
            continue;
        }
        if (CurrentChar + 1 < this->Raw.c_str() + this->Raw.size() && *CurrentChar == '?' && *(CurrentChar + 1) == '?') {
            this->Bytes.push_back(std::nullopt);
            CurrentChar += 2;
        }
        else if (CurrentChar + 1 < this->Raw.c_str() + this->Raw.size() && std::isxdigit(*CurrentChar) && std::isxdigit(*(CurrentChar + 1))) {
            uint8_t byte = static_cast<uint8_t>(std::stoi(this->Raw.substr(CurrentChar - this->Raw.c_str(), 2), nullptr, 16));
            this->Bytes.push_back(byte);
            CurrentChar += 2;
        }
        else {
            // 格式错误，遇到无法解析的字符，停止解析
            throw std::runtime_error("Invalid memory pattern format: " + this->Raw);
        }
    }
}