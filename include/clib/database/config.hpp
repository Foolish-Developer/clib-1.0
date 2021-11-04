#ifndef CLIB_DATABASE_CONFIG_HPP
#define CLIB_DATABASE_CONFIG_HPP

#include <string>
#include <cstdint>

namespace clib {
	namespace db {
		struct Config {
			std::string hostname;
			std::string userName;
			std::string password;
			std::string schema;
			int32_t port;
			bool autoReconnect;
			int32_t connectTimeout; // The unit is seconds. Set 0 to infinity timeout
			int16_t maxConnections;

			Config(std::string&& hostname,
				   std::string&& userName,
				   std::string&& password,
				   std::string&& schema,
				   int32_t port,
				   int16_t maxConnections,
				   bool autoReconnect = true,
				   int32_t connectTimeout = 60) noexcept
				: hostname{ std::forward<std::string>(hostname) },
				  userName{ std::forward<std::string>(userName) },
				  password{ std::forward<std::string>(password) },
				  schema{ std::forward<std::string>(schema) },
				  port{ port },
				  maxConnections{ maxConnections },
				  autoReconnect{ autoReconnect },
				  connectTimeout{ connectTimeout } {
				if (connectTimeout == 0) {
					this->connectTimeout = INT32_MAX;
				}
			}
		};
	}
}


#endif // !CLIB_DATABASE_CONFIG_HPP
