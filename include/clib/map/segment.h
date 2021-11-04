#ifndef CLIB_MAP_SEGMENT_H
#define CLIB_MAP_SEGMENT_H

#include <atomic>
#include <mutex>

namespace clib {
	class Segment {
	private:
		std::atomic<uint32_t> timestamp;
		std::mutex mutex;

	public:
		Segment(Segment&& other) = delete;
		Segment(const Segment& other) = delete;

		Segment& operator=(Segment&& other) = delete;
		Segment& operator=(const Segment& other) = delete;

		Segment() noexcept;
		~Segment() noexcept = default;

		void lock() noexcept;
		void unlock() noexcept;

		void addTimestamp() noexcept;
		uint32_t getTimestamp() noexcept;
	};
}

#endif // !CLIB_MAP_SEGMENT_H
