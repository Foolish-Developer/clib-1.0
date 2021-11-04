#include "clib/util.hpp"
#include "clib/datetime.h"
#include "clib/log/logger.h"
#include "clib/log/worker/console.h"
#include "clib/log/worker/file.h"

using namespace clib;
using namespace log;

Logger Logger::Instance;

void writeLog(bool& shutdown, File& file, Console& console, std::mutex& mutex, std::condition_variable& cond, queue::Mpsc<Message, true>& queue) {
	bool ok;
	Message msg;
	uint8_t retry{ 10 };
	std::string levelName[5] { "Info ", "Warn ", "Error", "Debug", "Fatal" };

	while (true) {
	begin:
		if (shutdown) {
			break;
		}

		std::tie(ok, msg) = queue.tryPop();
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

		// Write log
		if (msg.type == 0) {
			console.log(levelName[msg.level], msg.content, Datetime{});
		}
		else {
			file.log(levelName[msg.level], msg.content, Datetime{});
		}
	}

	// Write logs which already in the queue before receiving the signal exit
	while (true) {
		std::tie(ok, msg) = queue.tryPop();
		if (!ok) {
			return;
		}
		if (msg.type == 0) {
			console.log(levelName[msg.level], msg.content, Datetime{});
		}
		else {
			file.log(levelName[msg.level], msg.content, Datetime{});
		}
	}
}

//==============================
// Class "Logger" implementation
//==============================

Logger::Logger() 
	: queue{},
	  shutdown{ false },
	  file{ "" },
	  console{},
	  thread{},
	  mutex{},
	  cond{} {}

void Logger::init(const std::string& directory) {
	// Init workers
	this->file = File{ directory };

	this->queue.setSize(1024);
	this->thread = std::thread(
		writeLog, 
		std::ref(this->shutdown),
		std::ref(this->file),
		std::ref(this->console),
		std::ref(this->mutex),
		std::ref(this->cond),
		std::ref(this->queue)
	);
}

void Logger::log(Level level, std::string&& content) noexcept {
	this->queue.push(0, level, std::forward<std::string>(content));
	this->cond.notify_one();
}

void Logger::log_If(Level level, bool expression, std::string&& content) noexcept {
	if (expression) {
		this->queue.push(0, level, std::forward<std::string>(content));
		this->cond.notify_one();
	}
}

void Logger::logf(Level level, std::string&& content) {
	this->queue.push(1, level, std::forward<std::string>(content));
	this->cond.notify_one();
}

void Logger::logf_If(Level level, bool expression, std::string&& content) {
	if (expression) {
		this->queue.push(1, level, std::forward<std::string>(content));
		this->cond.notify_one();
	}
}

void Logger::exit() noexcept {
	// Use "lock" to guarantee no threads can wait after "notify_all"
	{
		std::unique_lock<std::mutex> lock(mutex);
		this->shutdown = true;
	}
	this->cond.notify_one();
	this->thread.join();
}