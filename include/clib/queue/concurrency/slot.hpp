#ifndef CLIB_QUEUE_SLOT_HPP
#define CLIB_QUEUE_SLOT_HPP

#include <atomic>
#include <type_traits>

#include "clib/util.hpp"

namespace clib {
	namespace queue {
        template<typename Type, bool UsingCPUCacheLine>
        class Slot {
        private:
            struct AtomNormal {
                std::atomic<size_t> value;

                AtomNormal(const size_t v)
                    : value{ v } {}

                size_t load(const std::memory_order order) const noexcept {
                    return this->value.load(order);
                }

                void store(const size_t v, const std::memory_order order) noexcept {
                    this->value.store(v, order);
                }
            };

            struct AtomCPUCacheLine {
                alignas(CPU_CACHE_LINE_SIZE) std::atomic<size_t> value;

                AtomCPUCacheLine(const size_t v)
                    : value{ v } {}

                size_t load(const std::memory_order order) const noexcept {
                    return this->value.load(order);
                }

                void store(const size_t v, const std::memory_order order) noexcept {
                    this->value.store(v, order);
                }
            };

        public:
            using Atom_t = typename std::conditional<UsingCPUCacheLine, AtomCPUCacheLine, AtomNormal>::type;
            Atom_t turn{ 0 };

        private:
            using Storage_t = typename std::aligned_storage<sizeof(Type), alignof(Type)>::type;
            Storage_t storage{};

        public:
            ~Slot() noexcept {
                // "turn" is odd number that slot is written (or has data)
                if (this->turn.load(std::memory_order_acquire) & 1) {
                    destroy();
                }
            }

            template<typename... Args>
            void construct(Args&&... args) noexcept {
                new (&this->storage) Type(std::forward<Args>(args)...);
            }

            void destroy() noexcept {
                reinterpret_cast<Type*>(std::addressof(this->storage))->~Type();
            }

            Type&& get() noexcept {
                return std::move(*(reinterpret_cast<Type*>(std::addressof(this->storage))));
            }
        };
	}
}

#endif // !CLIB_QUEUE_SLOT_HPP
