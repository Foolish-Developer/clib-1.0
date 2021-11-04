#ifndef CLIB_DATABASE_RESULT_H
#define CLIB_DATABASE_RESULT_H

#include <memory>

#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/prepared_statement.h>

namespace clib {
	namespace db {
		using Rows = std::unique_ptr<sql::ResultSet>;

		class Result {
		private:
			std::unique_ptr<sql::PreparedStatement> pstmt;

		public:
			Result(const Result& other) = delete;
			Result& operator=(const Result& other) = delete;

			Result() noexcept;
			Result(std::unique_ptr<sql::PreparedStatement>&& pstmt) noexcept;
			Result(Result&& other) noexcept;
			~Result();

			Result& operator=(Result&& other) noexcept;

			Rows get();
			bool isMore() noexcept;
		};
	}
}

#endif // !CLIB_DATABASE_RESULT_H
