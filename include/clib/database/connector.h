#ifndef CLIB_DATABASE_CONNECTOR_H
#define CLIB_DATABASE_CONNECTOR_H

#include <memory>

#include "clib/error.h"
#include "clib/queue/bqueue.hpp"
#include "clib/database/config.hpp"
#include "clib/database/connection.hpp"

namespace clib {
	namespace db {
		class Connector {
		private:
			queue::BQueue<Connection> connections;
			bool autoReconnect;
			uint16_t capacity;

		private:
			Error check(const Connection& connection);

		public:
			template <typename... Args>
			using Callback = std::function<std::tuple<bool, Result>(const std::string&, Args&&...)>;

			Connector(Connector&& other) = delete;
			Connector(const Connector& other) = delete;

			Connector& operator=(Connector&& other) = delete;
			Connector& operator=(const Connector& other) = delete;

			Connector(const Config& cfg);
			~Connector() noexcept;

			Error run(std::function<Error(Connection&)>&& cmd);
		};
	}
}

#endif // !CLIB_DATABASE_CONNECTOR_H
