#ifndef CLIB_QUEUE_BASE_HPP
#define CLIB_QUEUE_BASE_HPP

#include <thread>
#include <memory>

#include "clib/util.hpp"
#include "clib/error.h"
#include "clib/util.hpp"
#include "clib/queue/concurrency/slot.hpp"

namespace clib {
	namespace queue {

        // This is Base class for queues
		template<typename Type, bool UsingCPUCacheLine>
		class Base {
        protected:
			using Allocator_t = std::allocator<Slot<Type, UsingCPUCacheLine>>;
			size_t capacity;
            Slot<Type, UsingCPUCacheLine>* slots;
			Allocator_t allocator;

		protected:
            // If run on 32-bits platform, "size_t" is uint32_t.
            // If run on 64-bits platform, "size_t" is uint64_t.
            constexpr size_t roundupToPowerOf2(size_t n) const noexcept {
                --n;
                n |= n >> 1U;
                n |= n >> 2U;
                n |= n >> 4U;
                n |= n >> 8U;
                n |= n >> 16U;
                if (sizeof(size_t) == 8) {
                    n |= n >> 32U;
                }
                return ++n;
            }

            constexpr size_t idx(size_t i) const noexcept { return i & (this->capacity - 1U); }
            constexpr size_t turn(size_t i) const noexcept { return i / this->capacity; }

            void waitSlotReady(size_t turn, Slot<Type, UsingCPUCacheLine>& slot) noexcept {
                uint8_t retry = 10;
                while (turn != slot.turn.load(std::memory_order_acquire)) {
                    if (--retry) {
                        // Slowing down the “spin-wait” with the PAUSE instruction
                        PAUSE_CPU();
                    }
                    else {
                        // Sleep current thread and switch to another thread
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        retry = 10;
                    }
                }
            }

        public:
            Base(Base&& other) = delete;
            Base(const Base& other) = delete;

            Base& operator=(Base&& other) = delete;
            Base& operator=(const Base& other) = delete;

            Base() noexcept
                : capacity{ 0 },
                  slots{ nullptr },
                  allocator{} {}

            explicit Base(const size_t size)
                : capacity{ 0 },
                  slots{ nullptr },
                  allocator{} {
                setSize(size);
            }

            ~Base() noexcept {
                for (size_t i = 0; i < this->capacity; ++i) {
                    this->slots[i].~Slot();
                }
                this->allocator.deallocate(this->slots, this->capacity);
            }

            Error setSize(const size_t size) {
                if (this->capacity > 0) {
                    return Error{ GET_FUNC_NAME(), "The queue is not able to be created twice times" };
                }

                static_assert(
                    std::is_nothrow_default_constructible<Type>::value &&
                    (std::is_nothrow_move_constructible<Type>::value ||
                    std::is_nothrow_copy_constructible<Type>::value),
                    "Type must have noexcept default constructor and (move constructor and move assigment) or copy constructor"
                );

                static_assert(
                    std::is_nothrow_destructible<Type>::value,
                    "Type must have noexcept destructor"
                );

                static_assert(
                    !UsingCPUCacheLine || alignof(Slot<Type, UsingCPUCacheLine>) % CPU_CACHE_LINE_SIZE == 0,
                    "Type size must be a multiple of CPU cache line when using CPU cache line mode"
                );

                if (size == 0) {
                    throw "Capacity must be larger than 0";
                }

                this->capacity = roundupToPowerOf2(size);
                this->slots = this->allocator.allocate(this->capacity);

                // Verify alignment
                if (reinterpret_cast<size_t>(this->slots) % alignof(Slot<Type, UsingCPUCacheLine>) != 0) {
                    this->allocator.deallocate(this->slots, this->capacity);
                    throw "Memory allocation is failed";
                }

                // Initialize items
                for (size_t i = 0; i < this->capacity; ++i) {
                    new (&this->slots[i]) Slot<Type, UsingCPUCacheLine>{};
                }
                return Error{};
            }
		};
	}
}

#endif // !CLIB_QUEUE_BASE_HPP
