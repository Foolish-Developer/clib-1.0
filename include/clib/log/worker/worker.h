#ifndef CLIB_WORKER_H
#define CLIB_WORKER_H

#include <string>

#include "clib/datetime.h"

namespace clib {
	namespace log {
		class Worker {
		public:
			virtual void log(const std::string& level, const std::string& content, const Datetime& datetime) = 0;
		};
	}
}

#endif // !CLIB_WORKER_H
