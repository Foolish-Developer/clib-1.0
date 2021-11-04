#ifndef CLIB_SIGNAL_H
#define CLIB_SIGNAL_H

#include <tuple>
#include <functional>
#if defined(__GNUC__) || defined(__clang__)
#include <signal.h>
#include <unistd.h>
#include <string.h>
#elif defined(_MSC_VER)
#include <csignal>
#endif

#include "clib/error.h"

enum Order : uint8_t {
	HEAD   = 0U,
	MIDDLE = 1U,
	TAIL   = 2U,
};

enum Type : uint8_t {
	SIG_ARBT = 0U,
	SIG_FPE  = 1U,
	SIG_ILL  = 2U,
	SIG_INT  = 3U,
	SIG_SEGV = 4U,
	SIG_TERM = 5U,
};

namespace clib {
	class Signal {
	public:
		struct Info {
			uint64_t id;
			uint8_t order;
			std::function<void(int)> callback;

			Info(uint64_t id, uint8_t order, std::function<void(int)>&& callback) noexcept
				: id{ id },
				  order{ order },
				  callback{ callback } {}

			~Info() noexcept = default;
		};

	public:
		static Signal Instance;

	private:
		Signal() = default;

		Error pushHead(Type type, uint64_t id, Order order, std::function<void(int)>&& callback) noexcept;
		Error pushTail(Type type, uint64_t id, Order order, std::function<void(int)>&& callback) noexcept;
		Error pushMiddle(Type type, uint64_t id, Order order, std::function<void(int)>&& callback) noexcept;

	public:
		Signal(Signal&& other) = delete;
		Signal(const Signal& other) = delete;
		~Signal() noexcept = default;

		Signal& operator=(Signal&& other) = delete;
		Signal& operator=(const Signal& other) = delete;

		// Just call one time
		void init();

		Error reset(Type type);

		std::tuple<Error, uint64_t> subscribe(Type type, Order order, std::function<void(int)>&& callback) noexcept;
		Error unsubscribe(Type type, uint64_t id) noexcept;
	};
}


#endif // !CLIB_SIGNAL_H
