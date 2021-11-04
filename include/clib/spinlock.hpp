#ifndef CLIB_SPINLOCK_HPP
#define CLIB_SPINLOCK_HPP

#include <atomic>
#include <type_traits>

#include "clib/util.hpp"

namespace clib {
    template<bool UsingCPUCacheLine = false>
    class SpinLock {
    private:
        struct AtomNormal {
            std::atomic<bool> value;
        };

        struct AtomCPUCacheLine {
            alignas(CPU_CACHE_LINE_SIZE) std::atomic<bool> value;
        };
        
        using Atom_t = typename std::conditional<UsingCPUCacheLine, AtomCPUCacheLine, AtomNormal>::type;
        Atom_t key;

    public:
        SpinLock(SpinLock&& other) = delete;
        SpinLock(const SpinLock& other) = delete;

        SpinLock& operator=(SpinLock&& other) = delete;
        SpinLock& operator=(const SpinLock& other) = delete;

        SpinLock() noexcept 
            : key{ false } {};

        ~SpinLock() noexcept = default;


        void lock() noexcept {
            while (true) {
                // Optimistically assume the lock is free on the first try
                if (!this->key.value.exchange(true, std::memory_order_acquire)) {
                    return;
                }
                // Wait for lock to be released without generating cache misses
                while (this->key.value.load(std::memory_order_relaxed)) {
                    // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
                    // hyper-threads
                    PAUSE_CPU();
                }
            }
        }

        void unlock() noexcept {
            this->key.value.store(false, std::memory_order_release);
        }
    };
}

#endif // !CLIB_SPINLOCK_HPP
