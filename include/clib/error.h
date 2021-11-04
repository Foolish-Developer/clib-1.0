#ifndef CLIB_ERROR_H
#define CLIB_ERROR_H

#include <string>

namespace clib {
	class Error {
	private:
		struct Information {
			std::string funcName;
			std::string content;
		};

	private:
		Information* info;

	public:
		Error() noexcept;
		Error(std::string&& funcName, std::string&& content) noexcept;
		Error(Error&& other) noexcept;
		Error(const Error& other);
		~Error() noexcept;

		Error& operator=(Error&& other) noexcept;
		Error& operator=(const Error& other);

		bool nil() const noexcept;
		std::string toString() const noexcept;
	};
}

#endif // !CLIB_ERROR_H
