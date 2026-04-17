#pragma once

#include <cstddef>
#include <type_traits>

namespace MulNX {
    template<typename T>
    concept Pod = std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>;

    template<typename T, size_t size>
    concept PodSize = Pod<T> && sizeof(T) == size;

    template<typename T, size_t min, size_t max>
    concept PodSizeIn = Pod<T> && sizeof(T) > min && sizeof(T) <= max;
}