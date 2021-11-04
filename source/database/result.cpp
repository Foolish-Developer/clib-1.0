#include "clib/database/result.h"

using namespace clib;
using namespace db;

Result::Result() noexcept
	: pstmt{} {}

Result::Result(std::unique_ptr<sql::PreparedStatement>&& pstmt) noexcept
	: pstmt{ std::forward<std::unique_ptr<sql::PreparedStatement>>(pstmt) } {}

Result::~Result() {
	// Complete get results
	this->pstmt->getMoreResults();
	this->pstmt->close();
}

Result::Result(Result&& other) noexcept
	: pstmt{ std::move(other.pstmt) } {}

Result& Result::operator=(Result&& other) noexcept {
	this->pstmt = std::move(other.pstmt);
	return *this;
}

Rows Result::get() {
	return Rows{ this->pstmt->getResultSet() };
}

bool Result::isMore() noexcept {
	return this->pstmt->getMoreResults();
}