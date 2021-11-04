#ifndef CLIB_MAP_BUCKET_HPP
#define CLIB_MAP_BUCKET_HPP

#include "clib/util.hpp"
#include "clib/map/data.hpp"

namespace clib {
	template<class KType, class VType, bool StoreHash, class KEqual>
	class Bucket {
	private:
		using H = std::conditional<StoreHash, uint32_t, uint8_t>::type;
		H h;
		Data<KType, VType, KEqual> data;

	public:
		Bucket(Bucket&& other) = delete;
		Bucket(const Bucket& other) = delete;

		Bucket& operator=(Bucket&& other) = delete;
		Bucket& operator=(const Bucket& other) = delete;

		Bucket() noexcept
			: h{ 0U },
			  data{} {};

		~Bucket() noexcept {
			if (this->h > 0U) {
				this->data.~Data();
			}
		}

		template <bool TStoreHash = StoreHash, typename std::enable_if<TStoreHash>::type* = nullptr>
		void construct(const uint32_t& hash, KType&& k, VType&& v) noexcept {
			this->h = hash;
			this->data.construct(std::forward<KType>(k), std::forward<VType>(v));
		}

		template <bool TStoreHash = StoreHash, typename std::enable_if<!TStoreHash>::type* = nullptr>
		void construct(const uint32_t& hash, KType&& k, VType&& v) noexcept {
			this->h = 1U;
			this->data.construct(std::forward<KType>(k), std::forward<VType>(v));
		}

		void destruct() noexcept {
			this->h = 0U;
			this->data.destruct();
		}

		template <bool TStoreHash = StoreHash, typename std::enable_if<TStoreHash>::type* = nullptr>
		bool equal(const KType& k, const uint32_t& h) noexcept {
			return h == this->h && this->data.equal(k);
		}

		template <bool TStoreHash = StoreHash, typename std::enable_if<!TStoreHash>::type* = nullptr>
		bool equal(const KType& k, const uint32_t& h) noexcept {
			return this->data.equal(k);
		}

		void moveDataTo(Bucket& other) noexcept {
			other.h = this->h;
			this->data.swap(other.data);
			this->h = 2U;
		}

		bool isEmpty() noexcept {
			return this->h == 0U;
		}

		bool isSwap() noexcept {
			return this->h == 2U;
		}

		void use() noexcept {
			this->h = 1U;
		}

		const uint32_t& getHopInfo() noexcept {
			return this->data.getHopInfo();
		}

		std::shared_ptr<VType> getValue() noexcept {
			return this->data.getValue();
		}

		void onIndex(uint8_t idx) noexcept {
			this->data.onIndex(idx);
		}

		void offIndex(uint8_t idx) noexcept {
			this->data.offIndex(idx);
		}

		void lock() noexcept {
			this->data.lock();
		}

		void unlock() noexcept {
			this->data.unlock();
		}
	};
}

#endif // !CLIB_MAP_BUCKET_HPP
