#ifndef CLIB_THREAD_POOL_H
#define CLIB_THREAD_POOL_H

#include <mutex>
#include <thread>
#include <functional>
#include <condition_variable>

#include "clib/queue/concurrency/mpmc.hpp"

namespace clib {
	using Task = std::function<void()>;

	class Threadpool {
	public:
		static Threadpool Instance;

	private:
		queue::Mpmc<Task, true> queue;		// possess 2 cache lines
		bool shutdown;						//==|
		size_t poolSize;					//==|=> possess 1 cache line
		std::thread* threads;				//==|
		std::mutex mutex;					//==|
		std::condition_variable cond;		// possess 1 cache line

	private:
		Threadpool();

	public:
		Threadpool(Threadpool&& other) = delete;
		Threadpool(const Threadpool& other) = delete;
		~Threadpool() noexcept;

		Threadpool& operator=(Threadpool&& other) = delete;
		Threadpool& operator=(const Threadpool& other) = delete;

		void init();
		bool push(Task&& task) noexcept;
		void exit() noexcept;
	};
}

#endif // !CLIB_THREAD_POOL_H
