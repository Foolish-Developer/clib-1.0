#ifndef CLIB_BQUEUE_HPP
#define CLIB_BQUEUE_HPP

#include <mutex>
#include <tuple>
#include <type_traits>
#include <condition_variable>

#include "clib/list.hpp"

namespace clib {
	namespace queue {
		template<typename Type>
		class BQueue {
		private:
			List<Type> queue;
			std::mutex mutex;
			std::condition_variable cond;

		public:
			BQueue(const BQueue& other) = delete;
			BQueue& operator=(BQueue& other) = delete;

			BQueue() 
				: queue{},
				  mutex{},
				  cond{} {
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
			}

			BQueue(BQueue&& other) noexcept 
				: queue{ std::move(other.queue) },
				  mutex{ std::move(other.mutex) },
				  cond{ std::move(other.cond) } {}

			~BQueue() noexcept = default;

			BQueue& operator=(BQueue&& other) noexcept {
				this->queue = std::move(other.queue);
				this->mutex = std::move(other.mutex);
				this->cond = std::move(other.cond);
				return *this;
			}

			template<typename... Args>
			void push(Args&&... args) noexcept {
				{
					std::lock_guard<std::mutex> lock(this->mutex);
					this->queue.pushBack(std::forward<Args>(args)...);
				}
				this->cond.notify_one();
			}

			std::tuple<bool, Type> tryPop() noexcept {
				std::lock_guard<std::mutex> lock(this->mutex);
				if (this->queue.empty()) {
					return { false, Type{} };
				}
				return { true, this->queue.popFront() };
			}

			Type pop() noexcept {
				std::unique_lock<std::mutex> lock(this->mutex);
				while (this->queue.empty()) {
					this->cond.wait(lock);
				}
				return this->queue.popFront();
			}

			bool empty() const {
				std::lock_guard<std::mutex> lock(this->mutex);
				return this->queue.empty();
			}

			size_t size() const {
				std::lock_guard<std::mutex> lock(this->mutex);
				return this->queue.getSize();
			}
		};
	}
}

#endif // !CLIB_BQUEUE_HPP
