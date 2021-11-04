#include "clib/database/connector.h"

using namespace clib;
using namespace db;

Connector::Connector(const Config& cfg)
	: connections{},
	  autoReconnect{ cfg.autoReconnect },
	  capacity{ 0 } {
	this->capacity = cfg.maxConnections;
	if (this->capacity <= 0) {
		this->capacity = 3;
	}
	
	sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
	sql::ConnectOptionsMap connection_properties;
	sql::Connection* connection;

	connection_properties["hostName"] = cfg.hostname;
	connection_properties["userName"] = cfg.userName;
	connection_properties["password"] = cfg.password;
	connection_properties["schema"] = cfg.schema;
	connection_properties["port"] = sql::ConnectPropertyVal{ cfg.port };
	connection_properties["OPT_CONNECT_TIMEOUT"] = sql::ConnectPropertyVal{ cfg.connectTimeout };

	for (int16_t idx = 0; idx < this->capacity; ++idx) {
		// If connect failed will return exception
		connection = driver->connect(connection_properties);

		// Set max connections
		if (idx == 0) {
			std::unique_ptr<sql::PreparedStatement> pstmt;
			pstmt.reset(connection->prepareStatement("SET GLOBAL max_connections = ?;"));
			pstmt->setInt(1, this->capacity);
			pstmt->executeUpdate();
		}

		this->connections.push(Connection{ driver, connection });
	}
}

Connector::~Connector() noexcept {
	for (int16_t idx = 0; idx < this->capacity; ++idx) {
		this->connections.pop().close();
	}
}

Error Connector::run(std::function<Error(Connection&)>&& cmd) {
	auto connection = this->connections.pop();
	auto error = check(connection);
	if (error.nil()) {
		error = cmd(connection);
	}
	this->connections.push(std::move(connection));
	return error;
}

Error Connector::check(const Connection& connection) {
	if (connection.ping()) {
		return Error{};
	}

	auto retry = 3;
	if (!this->autoReconnect) {
		goto handleError;
	}
	
	while (!connection.ping() && (--retry));
	if (retry == 0) {
		goto handleError;
	}
	return Error{};
	
handleError:
	return Error{ GET_FUNC_NAME(), "The connection is disconnected with database" };
}