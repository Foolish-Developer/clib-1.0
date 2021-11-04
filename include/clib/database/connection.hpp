#ifndef CLIB_DATABASE_CONNECTION_HPP
#define CLIB_DATABASE_CONNECTION_HPP

#include <tuple>
#include <memory>
#include <string>
#include <functional>

#include <jdbc/mysql_driver.h>
#include <jdbc/cppconn/connection.h>
#include <jdbc/cppconn/prepared_statement.h>

#include "clib/util.hpp"
#include "clib/error.h"
#include "clib/database/result.h"

namespace clib {
	namespace db {
		class Connection {
		public:
			using Callback = std::function<Error(Result&&)>;

		private:
			friend class Connector;
			Callback callback;
			sql::mysql::MySQL_Driver* driver;
			std::unique_ptr<sql::Connection> connection;
			std::unique_ptr<sql::PreparedStatement> statement;

		private:
			bool ping() const {
				if (this->connection->isValid() || !this->connection->isClosed()) {
					return true;
				}
				return this->connection->reconnect();
			}

			void close() const {
				if (this->connection) {
					this->connection->close();
				}
			}

			void setParam(uint8_t index, const std::string& value) noexcept {
				this->statement->setString(index, value.c_str());
			}

			void setParam(uint8_t index, const char* value) noexcept {
				this->statement->setString(index, value);
			}

			void setParam(uint8_t index, std::istream* blob) noexcept {
				this->statement->setBlob(index, blob);
			}

			void setParam(uint8_t index, bool value) noexcept {
				this->statement->setBoolean(index, value);
			}

			void setParam(uint8_t index, double value) noexcept {
				this->statement->setDouble(index, value);
			}

			void setParam(uint8_t index, int32_t value) noexcept {
				this->statement->setInt(index, value);
			}

			void setParam(uint8_t index, uint32_t value) noexcept {
				this->statement->setUInt(index, value);
			}

			void setParam(uint8_t index, int64_t value) noexcept {
				this->statement->setInt64(index, value);
			}

			void setParam(uint8_t index, uint64_t value) noexcept {
				this->statement->setUInt64(index, value);
			}

#if defined(__GNUC__) || defined(__clang__)
			void setParam(uint8_t index, Callback& cb) noexcept {
				this->callback = cb;
			}
#elif defined(_MSC_VER)
			void setParam(uint8_t index, const Callback& cb) noexcept {
				this->callback = cb;
			}
#endif

			template <typename... Args>
			void setParams(Args&&... args) noexcept {
				uint8_t index = 1;
				([&](auto& arg) {
					this->setParam(index, arg);
					index++;
				} (args), ...);
			}

		public:
			Connection(const Connection& other) = delete;
			Connection& operator=(const Connection& other) = delete;

			Connection() noexcept
				: callback{ nullptr },
				  driver{ nullptr },
				  connection{ nullptr },
				  statement{ nullptr } {}

			Connection(sql::mysql::MySQL_Driver* driver, sql::Connection* connection) noexcept
				: callback{ nullptr },
				  driver{ driver },
				  connection{ connection },
				  statement{ nullptr } {
				this->connection->setAutoCommit(false);
			}

			Connection(Connection&& other) noexcept
				: callback{ std::move(other.callback) },
				  driver{ std::move(other.driver) },
				  connection{ std::move(other.connection) },
				  statement{ std::move(other.statement) } {}

			Connection& operator=(Connection&& other) noexcept {
				this->callback = std::move(other.callback);
				this->driver = std::move(other.driver);
				this->connection = std::move(other.connection);
				this->statement = std::move(other.statement);
				return *this;
			}

			void beginTask() {
				this->driver->threadInit();
			}

			void endTask() {
				this->driver->threadEnd();
			}
			
			template <typename... Args>
			Error transactionQuery(const std::string& statement, Args&&... args) {
				Error error{};
				try {
					this->statement.reset(this->connection->prepareStatement(statement));
					this->setParams(std::forward<Args>(args)...);

					if (!this->callback) {
						this->statement.reset();
						return Error{ GET_FUNC_NAME(), "Do not set callback to receivev database query results yet" };
					}

					this->statement->execute();
					error = this->callback(Result{ std::move(this->statement) });
					if (!error.nil()) {
						goto handleFailed;
					}

					this->connection->commit();
					this->callback = nullptr;
					return error;
				}
				catch (sql::SQLException& e) {
					error = Error{ GET_FUNC_NAME(), format("[MySQL] %d %s", e.getErrorCode(), e.what()) };
					goto handleFailed;
				}

			handleFailed:
				this->connection->rollback();
				this->statement.reset();
				this->callback = nullptr;
				return error;
			}

			template <typename... Args>
			Error transaction(const std::string& statement, Args&&... args) {
				try {
					this->statement.reset(this->connection->prepareStatement(statement));
					this->setParams(std::forward<Args>(args)...);

					this->statement->execute();
					this->connection->commit();
					this->statement.reset();
					return Error{};
				}
				catch (sql::SQLException& e) {
					this->connection->rollback();
					this->statement.reset();
					return Error{ GET_FUNC_NAME(), format("[MySQL] %d %s", e.getErrorCode(), e.what()) };
				}
			}
		};
	}
}

#endif // !CLIB_DATABASE_CONNECTION_HPP
