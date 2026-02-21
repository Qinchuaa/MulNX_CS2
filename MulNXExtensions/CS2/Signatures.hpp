#pragma once

#include "../WinExt/WinExt.hpp"

namespace MulNX {
    namespace CS2 {
        namespace Signatures {
            inline const static MulNX::Memory::Pattern CallIsPlayingDemo("48 8b 0d ?? ?? ?? ?? 48 8b 01 ff 90 48 01 00 00 0f 57 ff 84 c0 74 63 ba ff ff ff ff");
        }
    }
}