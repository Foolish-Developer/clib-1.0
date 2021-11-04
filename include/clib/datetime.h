#ifndef CLIB_DATETIME_H
#define CLIB_DATETIME_H

#include <string>

#if defined(_MSC_VER)
#pragma warning(disable : 4996) // Disable warning when using "ctime" lib
#endif

namespace clib {
	class Datetime {
	private:
		int32_t sec;   // seconds after the minute - [0, 60] including leap second
		int32_t min;   // minutes after the hour - [0, 59]
		int32_t hour;  // hours since midnight - [0, 23]
		int32_t day;   // day of the month - [1, 31]
		int32_t mon;   // months since January - [1, 12]
		int32_t year;  // years since 1900

	public:
		Datetime() noexcept;
		Datetime(const Datetime& other) noexcept;
		Datetime(Datetime&& other) noexcept;
		~Datetime() noexcept = default;

		Datetime& operator=(const Datetime& other) noexcept;
		Datetime& operator=(Datetime&& other) noexcept;

		bool operator==(const Datetime& other) noexcept;
		bool operator>(const Datetime& other) noexcept;
		bool operator<(const Datetime& other) noexcept;

		int8_t compareDate(const Datetime& other) noexcept;
		int8_t compareTime(const Datetime& other) noexcept;

		uint32_t getYear() const noexcept;
		uint8_t getMonth() const noexcept;
		uint8_t getDay() const noexcept;
		uint8_t getHour() const noexcept;
		uint8_t getMin() const noexcept;
		uint8_t getSec() const noexcept;

		std::string toString() const;
	};
}

#endif // !CLIB_DATETIME_H
