#ifndef CLIB_QUEUE_CONCURRENCY_MPSC_HPP
#define CLIB_QUEUE_CONCURRENCY_MPSC_HPP

#include <atomic>

#include "clib/queue/concurrency/base.hpp"

namespace clib {
    namespace queue {
        // "UsingCPUCacheLine" will help prevent false sharing between CPUs, increase performance
        // But it can also quickly fill L1, L2, L3 of CPU cache
        // So users should consider the capacity of the queue when initializing
        template<typename Type, bool UsingCPUCacheLine = false>
        class Mpsc : public Base<Type, UsingCPUCacheLine> {
        private:
            alignas(CPU_CACHE_LINE_SIZE) size_t head;
            alignas(CPU_CACHE_LINE_SIZE) std::atomic<size_t> tail;

        public:
            Mpsc(Mpsc&& other) = delete;
            Mpsc(const Mpsc& other) = delete;

            Mpsc& operator=(Mpsc&& other) = delete;
            Mpsc& operator=(const Mpsc& other) = delete;

            Mpsc() noexcept
                : Base<Type, UsingCPUCacheLine>{},
                  head{ 0 },
                  tail{ 0 } {}

            explicit Mpsc(const size_t size)
                : Base<Type, UsingCPUCacheLine>{ size },
                  head{ 0 },
                  tail{ 0 } {}

            virtual ~Mpsc() noexcept {}

            template <typename... Args>
            bool tryPush(Args&&... args) noexcept {
                auto tail = this->tail.load(std::memory_order_acquire);
                while (true) {
                    auto turn = this->turn(tail) << 1U;
                    auto& slot = this->slots[this->idx(tail)];

                    if (turn == slot.turn.load(std::memory_order_acquire)) {
                        if (this->tail.compare_exchange_strong(tail, tail + 1, std::memory_order_acq_rel)) {
                            slot.construct(std::forward<Args>(args)...);
                            slot.turn.store(turn + 1U, std::memory_order_release);
                            return true;
                        }
                    }
                    else {
                        auto const preTail = tail;
                        tail = this->tail.load(std::memory_order_acquire);
                        if (tail == preTail) {
                            return false;
                        }
                    }
                }
            }

            template <typename... Args>
            void push(Args&&... args) noexcept {
                auto tail = this->tail.fetch_add(1, std::memory_order_relaxed);
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

#endif // !CLIB_QUEUE_CONCURRENCY_MPSC_HPP
