#include <ctime>

#include "clib/util.hpp"
#include "clib/datetime.h"

using namespace clib;

Datetime::Datetime() noexcept {
	tm time;
	time_t now = std::time(nullptr);
	localtime_s(&time, &now);

	this->sec = time.tm_sec;
	this->min = time.tm_min;
	this->hour = time.tm_hour;
	this->day = time.tm_mday;
	this->mon = time.tm_mon + 1;
	this->year = time.tm_year + 1900;
}

Datetime::Datetime(Datetime&& other) noexcept 
	: sec{ std::move(other.sec) },
	  min{ std::move(other.min) }, 
	  hour{ std::move(other.hour) }, 
	  day{ std::move(other.day) }, 
	  mon{ std::move(other.mon) }, 
	  year{ std::move(other.year) } {}

Datetime::Datetime(const Datetime& other) noexcept
	: sec{ other.sec },
	  min{ other.min },
	  hour{ other.hour },
	  day{ other.day },
	  mon{ other.mon },
	  year{ other.year } {}

Datetime& Datetime::operator=(Datetime&& other) noexcept {
	this->sec = std::move(other.sec);
	this->min = std::move(other.min);
	this->hour = std::move(other.hour);
	this->day = std::move(other.day);
	this->mon = std::move(other.mon);
	this->year = std::move(other.year);
	return *this;
}

Datetime& Datetime::operator=(const Datetime& other) noexcept {
	this->sec = other.sec;
	this->min = other.min;
	this->hour = other.hour;
	this->day = other.day;
	this->mon = other.mon;
	this->year = other.year;
	return *this;
}

bool Datetime::operator==(const Datetime& other) noexcept {
	return compareDate(other) == 0 && compareTime(other) == 0;
}

bool Datetime::operator>(const Datetime& other) noexcept {
	auto res = compareDate(other);
	return res > 0 || (res == 0 && compareTime(other) > 0);
}

bool Datetime::operator<(const Datetime& other) noexcept {
	auto res = compareDate(other);
	return res < 0 || (res == 0 && compareTime(other) < 0);
}

int8_t Datetime::compareDate(const Datetime& other) noexcept {
	uint32_t unit1 = this->year * 10000 + this->mon * 100 + this->day;
	uint32_t unit2 = other.year * 10000 + other.mon * 100 + other.day;
	if (unit1 < unit2) {
		return -1;
	}
	return unit1 > unit2;
}

int8_t Datetime::compareTime(const Datetime& other) noexcept {
	uint32_t unit1 = this->hour * 10000 + this->min * 100 + this->sec;
	uint32_t unit2 = other.hour * 10000 + other.min * 100 + other.sec;
	if (unit1 < unit2) {
		return -1;
	}
	return unit1 > unit2;
}

uint32_t Datetime::getYear() const noexcept {
	return this->year;
}

uint8_t Datetime::getMonth() const noexcept {
	return this->mon;
}

uint8_t Datetime::getDay() const noexcept {
	return this->day;
}

uint8_t Datetime::getHour() const noexcept {
	return this->hour;
}

uint8_t Datetime::getMin() const noexcept {
	return this->min;
}

uint8_t Datetime::getSec() const noexcept {
	return this->sec;
}

std::string Datetime::toString() const {
	// yyyy-mm-dd HH:MM:SS
	return format(
		"%d-%02d-%02d %02d:%02d:%02d",
		year,
		mon,
		day,
		hour,
		min,
		sec
	);
}