#ifndef CLIB_LOG_MESSAGE_H
#define CLIB_LOG_MESSAGE_H

#include <string>
#include <memory>
#include <cstdint>

#include "clib/log/worker/worker.h"

namespace clib {
	namespace log {
		class Message {
		public:
			int8_t type;
			int8_t level;
			std::string content;

		public:
			Message() noexcept;
			Message(int8_t type, int8_t level, std::string&& content) noexcept;
			Message(Message&& other) noexcept;
			Message(const Message& other) = delete;
			~Message() noexcept = default;

			Message& operator=(Message&& other) noexcept;
			Message& operator=(const Message& other) = delete;
		};

	}
}

#endif // !CLIB_LOG_MESSAGE_H
