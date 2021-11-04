#ifndef CLIB_LOG_WORKER_CONSOLE_H
#define CLIB_LOG_WORKER_CONSOLE_H

#include "clib/log/worker/worker.h"

namespace clib {
	namespace log {
		class Console : public Worker {
		public:
			Console() noexcept = default;
			Console(Console&& other) noexcept = default;
			Console(const Console& other) = delete;
			~Console() noexcept = default;

			Console& operator=(Console&& other) noexcept = default;
			Console& operator=(const Console& other) = delete;

			void log(const std::string& level, const std::string& content, const Datetime& datetime) noexcept;
		};
	}
}

#endif // !CLIB_LOG_WORKER_CONSOLE_H
