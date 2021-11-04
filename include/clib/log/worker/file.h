#ifndef CLIB_LOG_WORKER_FILE_H
#define CLIB_LOG_WORKER_FILE_H

#include <cstdio>

#include "clib/datetime.h"
#include "clib/log/worker/worker.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4996) // Disable warning when using "std::FILE*"
#endif

namespace clib {
	namespace log {
		class File : public Worker {
		private:
			Datetime now;
			std::FILE* file;
			std::string directory;

		private:
			void makeFile(const Datetime& datetime);

		public:
			File(const std::string& directory);
			File(File&& other) noexcept;
			File(const File& other) = delete;
			~File() noexcept;

			File& operator=(File&& other) noexcept;
			File& operator=(const File& other) = delete;

			void log(const std::string& level, const std::string& content, const Datetime& datetime) noexcept;
		};
	}
}

#endif // !CLIB_LOG_WORKER_FILE_H
