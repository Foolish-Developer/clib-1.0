#ifndef CLIB_QUEUE_CONCURRENCY_SPSC_HPP
#define CLIB_QUEUE_CONCURRENCY_SPSC_HPP

#include <tuple>

#include "clib/queue/concurrency/base.hpp"

namespace clib {
    namespace queue {
        // "UsingCPUCacheLine" will help prevent false sharing between CPUs, increase performance
        // But it can also quickly fill L1, L2, L3 of CPU cache
        // So users should consider the capacity of the queue when initializing
        template<typename Type, bool UsingCPUCacheLine = false>
        class Spsc : public Base<Type, UsingCPUCacheLine> {
        private:
            alignas(CPU_CACHE_LINE_SIZE) size_t head;
            alignas(CPU_CACHE_LINE_SIZE) size_t tail;

        public:
            Spsc(Spsc&& other) = delete;
            Spsc(const Spsc& other) = delete;

            Spsc& operator=(Spsc&& other) = delete;
            Spsc& operator=(const Spsc& other) = delete;

            Spsc() noexcept
                : Base<Type, UsingCPUCacheLine>{},
                  head{ 0 },
                  tail{ 0 } {}

            explicit Spsc(const size_t size)
                : Base<Type, UsingCPUCacheLine>{ size },
                  head{ 0 },
                  tail{ 0 } {}

            virtual ~Spsc() noexcept {}

            template <typename... Args>
            bool tryPush(Args&&... args) noexcept {
                auto tail = this->tail;
                auto turn = this->turn(tail) << 1U;
                auto& slot = this->slots[this->idx(tail)];

                if (turn != slot.turn.load(std::memory_order_acquire)) {
                    return false;
                }

                this->tail++;
                slot.construct(std::forward<Args>(args)...);
                slot.turn.store(turn + 1U, std::memory_order_release);
                return true;
            }

            template <typename... Args>
            void push(Args&&... args) noexcept {
                auto tail = this->tail++;
                auto turn = this->turn(tail) << 1U;
                auto& slot = this->slots[this->idx(tail)];

                if (turn != slot.turn.load(std::memory_order_acquire)) {
                    Base<Type, UsingCPUCacheLine>::waitSlotReady(turn, slot);
                }

                slot.construct(std::forward<Args>(args)...);
                slot.turn.store(turn + 1U, std::memory_order_release);
            }

            std::tuple<bool, Type> tryPop() noexcept {
                auto head = this->head;
                auto turn = (this->turn(head) << 1U) + 1U;
                auto& slot = this->slots[this->idx(head)];

                if (turn != slot.turn.load(std::memory_order_acquire)) {
                    return { false, Type{} };
                }

                this->head++;
                // Get result & Update slot
                std::tuple<bool, Type> result{ true, slot.get() };
                slot.destroy();
                slot.turn.store(turn + 1U, std::memory_order_release);
                return result;
            }

            Type pop() noexcept {
                auto head = this->head++;
                auto turn = (this->turn(head) << 1U) + 1U;
                auto& slot = this->slots[this->idx(head)];

                if (turn != slot.turn.load(std::memory_order_acquire)) {
                    Base<Type, UsingCPUCacheLine>::waitSlotReady(turn, slot);
                }

                // Get result & Update slot
                Type result{ slot.get() };
                slot.destroy();
                slot.turn.store(turn + 1U, std::memory_order_release);
                return result;
            }
        };
    }
}

#endif // !CLIB_QUEUE_CONCURRENCY_SPSC_HPP
