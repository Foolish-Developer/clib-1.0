#include "clib/log/message.h"

using namespace clib;
using namespace log;

Message::Message() noexcept
	: type{ -1 },
	  level{ -1 },
	  content{} {}

Message::Message(int8_t type, int8_t level, std::string&& content) noexcept
	: type{ type },
	  level{ level },
	  content{ std::forward<std::string>(content) } {}

Message::Message(Message&& other) noexcept 
	: type{ std::move(other.type) },
	  level{ std::move(other.level) },
	  content{ std::move(other.content) } {}

Message& Message::operator=(Message&& other) noexcept {
	this->type = std::move(other.type);
	this->level = std::move(other.level);
	this->content = std::move(other.content);
	return *this;
}