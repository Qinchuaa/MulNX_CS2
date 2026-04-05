#pragma once

#include <MulNXExtensions/WinExt/WinExt.hpp>

namespace MulNX {
    namespace CS2 {
        namespace Signatures {
            inline const static MulNX::Memory::Pattern CallIsPlayingDemo("48 8b 0d ?? ?? ?? ?? 48 8b 01 ff 90 48 01 00 00 0f 57 ff 84 c0 74 63 ba ff ff ff ff");
            inline const static MulNX::Memory::Pattern GetDecoratedPlayerName("44 89 44 24 18 48 89 54 24 10 55 53 56 57 41 54 41 55 41 56 41 57 48 8d ac 24 28 f5 ff ff");
        }
    }
}