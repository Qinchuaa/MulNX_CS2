#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "Math/Math.hpp"
#include "any_smart_ptr/any_smart_ptr.hpp"
#include "vmt/vmt.hpp"
#include "TripleBuffer/TripleBuffer.hpp"
#include "CharUtility/CharUtility.hpp"
#include "MulNXHandle/MulNXHandle.hpp"

#include "../Config/fwd.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>
#include <shared_mutex>
#include <atomic>
#include <thread>
#include <filesystem>
#include <functional>
#include <memory>
#include <variant>
#include <array>