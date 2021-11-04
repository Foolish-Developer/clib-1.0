#ifndef CLIB_MAP_DATA_HPP
#define CLIB_MAP_DATA_HPP

#include <memory>
#include <cstdint>
#include <type_traits>

#include "clib/spinlock.hpp"

namespace clib {
	template<class KType, class VType, class KEqual>
	class Data {
	private:
		using Pair_t = std::pair<KType, std::shared_ptr<VType>>;
		using Storage_t = typename std::aligned_storage<sizeof(Pair_t), alignof(Pair_t)>::type;

		SpinLock<false> spin;
		uint32_t hopInfo;
		Storage_t storage;

	private:
		Pair_t& getData() noexcept {
			return *reinterpret_cast<Pair_t*>(std::addressof(storage));
		}

	public:
		Data(Data&& other) = delete;
		Data(const Data& other) = delete;

		Data& operator=(Data&& other) = delete;
		Data& operator=(const Data& other) = delete;

		Data() noexcept
			: spin{},
			  hopInfo{ 0U },
			  storage{} {};

		~Data() noexcept = default;

		bool equal(const KType& k) noexcept {
			return KEqual()(getData().first, k);
		}

		void swap(Data& other) noexcept {
			std::swap(getData(), other.getData());
		}

		void construct(KType&& k, VType&& v) noexcept {
			new (&this->storage) Pair_t{
				KType{ std::forward<KType>(k) },
				std::shared_ptr<VType>{ new VType{ std::forward<VType>(v) } }
			};
		}

		void destruct() noexcept {
			getData().~Pair_t();
		}

		const uint32_t& getHopInfo() noexcept {
			return this->hopInfo;
		}

		std::shared_ptr<VType> getValue() noexcept {
			return getData().second;
		}

		void onIndex(uint8_t idx) noexcept {
			this->hopInfo |= (1U << idx);
		}

		void offIndex(uint8_t idx) noexcept {
			this->hopInfo &= ~(1U << idx);
		}

		void lock() noexcept {
			this->spin.lock();
		}

		void unlock() noexcept {
			this->spin.unlock();
		}
	};
}

#endif // !CLIB_MAP_DATA_HPP
