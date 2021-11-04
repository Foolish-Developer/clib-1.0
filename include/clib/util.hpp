#ifndef CLIB_UTIL_HPP
#define CLIB_UTIL_HPP

#include <new>
#include <chrono>
#include <string>
#include <memory>
#include <string>
#include <iostream>
#include <stdexcept>

#if defined(__GNUC__) || defined(__clang__)
static std::string getMethodName(const std::string& name) noexcept {
    size_t begin = name.find_last_of("::");
    for (; begin > 0 && name[begin] != ' '; --begin);
    begin++;
    return name.substr(begin, name.rfind("(") - begin);
}

#define PAUSE_CPU() __builtin_ia32_pause()
#define GET_FUNC_NAME() getMethodName(__PRETTY_FUNCTION__)

#elif defined(_MSC_VER)
#include <emmintrin.h>

#define PAUSE_CPU() _mm_pause()
#define GET_FUNC_NAME() __FUNCTION__
#endif

#define CPU_CACHE_LINE_SIZE std::hardware_destructive_interference_size

namespace clib {
    template<typename ... Args>
    static std::string format(const std::string& format, Args&& ... args) {
        size_t size = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1ULL; // Extra space for '\0'
        if (size <= 0) {
            throw "Error during formatting.";
        }
        std::unique_ptr<char> buf{ new char[size] };
        std::snprintf(buf.get(), size, format.c_str(), args ...);
        return std::string{ buf.get(), buf.get() + size - 1 }; // We don't want the '\0' inside
    }

    static uint64_t getTimestamp() noexcept {
        const auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    }

    static size_t roundupToPowerOf2(size_t value) {
        if (value == 0) {
            return 1;
        }

        // If value is power of 2
        if ((value & (value - 1)) == 0) {
            return value;
        }

        --value;
        for (size_t i = 1, n = sizeof(size_t) * CHAR_BIT; i < n; i *= 2) {
            value |= value >> i;
        }
        return value + 1;
    }

    static int32_t findFirstSet(uint32_t x) {
        if (x == 0) {
            return -1;
        }

#if defined(__GNUC__) || defined(__clang__)
        return __builtin_ffs(x) - 1;
#elif defined(_MSC_VER)
        for (uint32_t idx = 31, mask = 1U << idx; idx >= 0; --idx, mask >>= 1U) {
            if (x & mask) return idx;
        }
        return -1;
#endif
    }
}

#endif // !CLIB_UTIL_HPP
