#include <functional>

#include "clib/threadpool.h"
#include "clib/log/logger.h"

using namespace clib;

void doTask(const uint8_t threadIdx, bool& shutdown, std::mutex& mutex, std::condition_variable& cond, queue::Mpmc<Task, true>& queue) {
	bool ok;
	Task task;
	uint8_t retry{ 10 };
	
	while (true) {
	begin:
		if (shutdown) {
			break;
		}

		std::tie(ok, task) = queue.tryPop();
		if (!ok) {
			// Retry before switching from user mode to kernel mode
			if (--retry) {
				PAUSE_CPU();
				goto begin;
			}
			// Switch to kernel mode and wait for a wake-up event
			std::unique_lock<std::mutex> lock(mutex);
			if (!shutdown) {
				cond.wait(lock); // "wait" will unlock
			}
			retry = 10;
			goto begin;
		}
		task();
	}

	// Write tasks which already in the queue before receiving the signal exit
	while (true) {
		std::tie(ok, task) = queue.tryPop();
		if (!ok) {
			break;
		}
		task();
	}
	log::Logger::Instance.logf(INFO, format("ThreadPool-Thread %d exited", threadIdx));
}

Threadpool Threadpool::Instance;

Threadpool::Threadpool() 
	: queue{},
	  shutdown{ false },
	  poolSize{},
	  threads{ nullptr },
	  mutex{},
	  cond{} {}

Threadpool::~Threadpool() {
	if (this->threads) {
		delete[] this->threads;
	}
}

void Threadpool::init() {
	this->queue.setSize(1024);

	this->poolSize = std::thread::hardware_concurrency();
	if (this->poolSize <= 0) {
		throw "Can not create threads for pool";
	}

	this->threads = new std::thread[this->poolSize];
	for (auto idx = 0; idx < this->poolSize; ++idx) {
		this->threads[idx] = std::thread(
			doTask,
			idx,
			std::ref(this->shutdown),
			std::ref(this->mutex),
			std::ref(this->cond),
			std::ref(this->queue)
		);
	}
}

bool Threadpool::push(Task&& task) noexcept {
	auto ok = this->queue.tryPush(std::forward<Task>(task));
	if (ok) {
		this->cond.notify_one();
	}
	return ok;
}

void Threadpool::exit() noexcept {
	if (this->threads == nullptr) {
		return;
	}

	// Use "lock" to guarantee no threads can wait after "notify_all"
	{
		std::unique_lock<std::mutex> lock(mutex);
		this->shutdown = true;
	}
	this->cond.notify_all();

	for (auto idx = 0; idx < this->poolSize; ++idx) {
		this->threads[idx].join();
	}
}