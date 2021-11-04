#include "clib/error.h"
#include "clib/util.hpp"

using namespace clib;

Error::Error() noexcept
	: info{ nullptr } {}

Error::Error(std::string&& funcName, std::string&& content) noexcept
	: info{ new Information{} } {
	this->info->funcName = std::forward<std::string>(funcName);
	this->info->content = std::forward<std::string>(content);
}

Error::Error(Error&& other) noexcept 
	: info{ other.info } {
	other.info = nullptr;
}

Error::Error(const Error& other) 
	: info{ new Information{} } {
	this->info->funcName = other.info->funcName;
	this->info->content = other.info->content;
}

Error::~Error() noexcept {
	if (this->info) {
		delete this->info;
	}
}

Error& Error::operator=(Error&& other) noexcept {
	if (this->info) {
		delete this->info;
	}
	if (!other.info) {
		this->info = nullptr;
		return *this;
	}
	this->info = new Information{};
	this->info->funcName = std::move(other.info->funcName);
	this->info->content = std::move(other.info->content);
	return *this;
}

Error& Error::operator=(const Error& other) {
	if (this->info) {
		delete this->info;
	}
	if (!other.info) {
		this->info = nullptr;
		return *this;
	}
	this->info = new Information{};
	this->info->funcName = other.info->funcName;
	this->info->content = other.info->content;
	return *this;
}

bool Error::nil() const noexcept {
	return this->info == nullptr;
}

std::string Error::toString() const noexcept {
	return format(
		"[%s]:%s", 
		this->info->funcName.c_str(),
		this->info->content.c_str()
	);
}