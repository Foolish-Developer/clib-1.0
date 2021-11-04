#ifndef CLIB_LOG_WORKER_H
#define CLIB_LOG_WORKER_H

#include <mutex>
#include <thread>
#include <condition_variable>

#include "clib/log/message.h"
#include "clib/log/worker/file.h"
#include "clib/log/worker/console.h"
#include "clib/queue/concurrency/mpsc.hpp"

enum Level : int8_t {
	INFO = 0,
	WARN = 1U,
	ERROR = 2U,
	DEBUG = 3U,
	FATAL = 4U,
};

namespace clib {
	namespace log {
		class Logger {
		public:
			static Logger Instance;

		private:
			queue::Mpsc<Message, true> queue;	// possess 1 cache line
			File file;							// possess 1 cache line
			Console console;					//==|
			bool shutdown;						//==|=> possess 1 cache line
			std::thread thread;					//==|
			std::mutex mutex;					//==|
			std::condition_variable cond;		// possess 1 cache line

		private:
			Logger();

		public:
			Logger(Logger&& other) = delete;;
			Logger(const Logger& other) = delete;
			~Logger() noexcept = default;

			Logger& operator=(Logger&& other) = delete;
			Logger& operator=(const Logger& other) = delete;

			void init(const std::string& directory);

			void log(Level level, std::string&& content) noexcept;
			void log_If(Level level, bool expression, std::string&& content) noexcept;

			void logf(Level level, std::string &&content);
			void logf_If(Level level, bool expression, std::string&& content);

			void exit() noexcept;
		};
	}
}

#endif // !CLIB_LOG_WORKER_H
