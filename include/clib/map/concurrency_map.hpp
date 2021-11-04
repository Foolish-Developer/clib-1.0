#ifndef CLIB_CONCURRENCY_MAP_HPP
#define CLIB_CONCURRENCY_MAP_HPP

#include <cmath>

#include "clib/map/segment.h"
#include "clib/map/hashf.hpp"
#include "clib/map/bucket.hpp"

#define HOP_RANGE 32U
#define ADD_RANGE 1024U
#define SEGMENT_RANGE 4096U
#define MAX_LOAD_FACTOR 0.82f

namespace clib {
	template<class KType, 
			 class VType, 
			 bool StoreHash = true, 
			 class KEqual = std::equal_to<KType>, 
			 class Hash = Hashf<KType>>
	class ConcurrencyMap {
	private:
		using Bucket_t = Bucket<KType, VType, StoreHash, KEqual>;
		using Pair_t = std::pair<KType, VType>;

		uint32_t segmentMask;
		uint32_t bucketMask;
		uint32_t capacity;
		Segment* segments;
		Bucket_t* buckets;

	private:
		bool findCloserFreeBucket(Segment*& begSegment, Bucket_t*& freeBucket, uint32_t& freeDist) {
			Bucket_t* homeMoveBucket{ freeBucket - (HOP_RANGE - 1U) };
			uint32_t t = (uint32_t)(freeBucket - this->buckets);
			uint32_t idxFreeBucket;
			for (idxFreeBucket = HOP_RANGE - 1; idxFreeBucket > 0; --idxFreeBucket, ++homeMoveBucket) {
				uint32_t mask{ 1U };
				int32_t idxMoveBucket{ -1 };
				uint32_t hopInfo{ homeMoveBucket->getHopInfo() };

				if (hopInfo == 0) {
					continue;
				}
				// GO FROM LEFT TO RIGHT TO FIND FIRST BIT 1 ...................
				for (uint32_t idx = 0; idx < idxFreeBucket; ++idx, mask <<= 1U) {
					if (mask & hopInfo) {
						idxMoveBucket = idx;
						break;
					}
				}
				if (idxMoveBucket != -1) {
					Segment* moveSegment;
					{
						uint32_t idxMoveSegment{ (uint32_t)(homeMoveBucket - this->buckets) & this->segmentMask };
						//uint32_t idxMoveSegment{ (uint32_t)(homeMoveBucket - this->buckets) / SEGMENT_RANGE };
						if (idxMoveSegment >= this->segmentMask + 1U) {
							idxMoveSegment = this->segmentMask;
						}

						moveSegment = &this->segments[idxMoveSegment];
						if (moveSegment != begSegment) {
							moveSegment->lock();
						}
					}

					// "hopInfo" can be changed ("set" or "remove"),
					// so we need to check before handling.
					if (homeMoveBucket->getHopInfo() == hopInfo) {
						Bucket_t* moveBucket{ homeMoveBucket + idxMoveBucket };
						moveBucket->lock();
						moveBucket->moveDataTo(*freeBucket);
						moveBucket->unlock();

						homeMoveBucket->onIndex(idxFreeBucket);
						homeMoveBucket->offIndex(idxMoveBucket);

						freeBucket = moveBucket;
						freeDist -= (idxFreeBucket - idxMoveBucket);

						if (moveSegment != begSegment) {
							moveSegment->unlock();
						}
						moveSegment->addTimestamp();
						return true;
					}
					if (moveSegment != begSegment) {
						moveSegment->unlock();
					}
				}
			}
			freeBucket = nullptr;
			freeDist = 0;
			return false;
		}

		bool doSet(Pair_t&& pair) {
			uint32_t hash;
			Segment* segment;
			Bucket_t* homeBucket;
			{
				uint32_t idxBegBucket;
				uint32_t idxSegment;

				hash = Hash()(pair.first);
				idxSegment = hash & this->segmentMask;
				idxBegBucket = hash & this->bucketMask;
				segment = &this->segments[idxSegment];
				segment->lock();
				homeBucket = &this->buckets[idxBegBucket];
			}

			// CHECK IF ALREADY CONTAIN ................
			{
				uint32_t hopInfo{ homeBucket->getHopInfo() };
				uint8_t idx;
				Bucket_t* currBucket;
				while (hopInfo != 0U) {
					idx = findFirstSet(hopInfo);
					currBucket = homeBucket + idx;
					if (currBucket->equal(pair.first, hash)) {
						currBucket->unlock();
						segment->unlock();
						return false;
					}
					hopInfo &= ~(1U << idx);
				}
			}

			// LOOK FOR FREE BUCKET ....................
			Bucket_t* freeBucket{ homeBucket };
			uint32_t freeDist{ 0U };
			for (; freeDist < ADD_RANGE; ++freeDist, ++freeBucket) {
				// "freeBucket" can be in another segment (and we don't lock this segment).
				// In other words, it can be accessed by another thread (on "doSet" or "remove" operation)
				freeBucket->lock();
				if (freeBucket->isEmpty()) {
					freeBucket->use();
					freeBucket->unlock();
					break;
				}
				freeBucket->unlock();
			}

			// PLACE THE NEW KEY .......................
			if (freeDist >= ADD_RANGE) {
				segment->unlock();
				return false;
			}
			
			bool res;
			do {
				if (freeDist < HOP_RANGE) {
					freeBucket->lock();
					freeBucket->construct(hash, std::move(pair.first), std::move(pair.second));
					freeBucket->unlock();
					homeBucket->onIndex(freeDist);
					res = true;
					break;
				}
				res = findCloserFreeBucket(segment, freeBucket, freeDist);
			} while (freeBucket != nullptr);
			segment->addTimestamp();
			segment->unlock();
			return res;
		}

	public:
		ConcurrencyMap()
			: segmentMask{ 0U },
			  bucketMask{ 0U },
			  capacity{ 0U },
			  segments{ nullptr },
			  buckets{ nullptr } {
			static_assert(
				std::is_nothrow_move_assignable<KType>::value && 
				std::is_nothrow_destructible<KType>::value &&
				std::is_nothrow_destructible<VType>::value,
				"KType must have noexcept move assigment and noexecpt destructor, VType must have noexecpt destructor"
			);
		}

		~ConcurrencyMap() noexcept {
			delete[] this->buckets;
			delete[] this->segments;
		}

		size_t init(uint32_t expectedBuckets, int32_t expectedSegments = 0) noexcept {
			size_t bucketSize{ roundupToPowerOf2(expectedBuckets) };
			size_t segmentSize;

			if (bucketSize < SEGMENT_RANGE && expectedSegments <= 0) {
				bucketSize = SEGMENT_RANGE;
			}

			if (expectedSegments > 0) {
				segmentSize = roundupToPowerOf2(expectedSegments);
			}
			else {
				segmentSize = bucketSize / SEGMENT_RANGE;
			}
			this->segmentMask = segmentSize - 1U;
			this->bucketMask = bucketSize - 1U;
			this->capacity = bucketSize + ADD_RANGE + 1U; // Fix: the last bucket can not find emtpy bucket.

			this->segments = new Segment[segmentSize];
			this->buckets = new Bucket_t[this->capacity];
			return std::round(bucketSize * MAX_LOAD_FACTOR);
		}

		template<typename... Args>
		bool set(Args&&... args) {
			return doSet(Pair_t(std::forward<Args>(args)...));
		}

		std::shared_ptr<VType> get(KType&& key) noexcept {
			return get(key);
		}

		std::shared_ptr<VType> get(const KType& key) noexcept {
			uint32_t hash;
			Segment* segment;
			Bucket_t* homeBucket;
			uint32_t hopInfo;
			{
				uint32_t idxSegment;
				uint32_t idxBegBucket;

				hash = Hash()(key);
				idxSegment = hash & this->segmentMask;
				idxBegBucket = hash & this->bucketMask;
				segment = &this->segments[idxSegment];
				homeBucket = &this->buckets[idxBegBucket];
				hopInfo = homeBucket->getHopInfo();

				if (hopInfo == 0U) { // Bucket empty or key not exist in range
					return std::shared_ptr<VType>{ nullptr };
				}
			}

			// CHECK IF ALREADY CONTAIN ................
			uint32_t timestamp{ segment->getTimestamp() };
			Bucket_t* currBucket;
			uint8_t idx;

		begin:
			hopInfo = homeBucket->getHopInfo();
			while (hopInfo != 0U) {
				idx = findFirstSet(hopInfo);
				currBucket = homeBucket + idx;
				currBucket->lock();
				if (!currBucket->isEmpty()) {		// is removed by another thread
					if (currBucket->isSwap()) {		// is swapped by another thread
						currBucket->unlock();
						goto begin;
					}
					if (currBucket->equal(key, hash)) {
						auto res = currBucket->getValue();
						currBucket->unlock();
						return res;
					}
				}
				currBucket->unlock();
				hopInfo &= ~(1U << idx);
			}

			// CHECK NEW ITEM IS ADDED ................
			if (timestamp == segment->getTimestamp()) {
				return std::shared_ptr<VType>{ nullptr };
			}
			return std::shared_ptr<VType>{ nullptr };
		}

		bool remove(KType&& key) {
			return remove(key);
		}

		bool remove(const KType& key) {
			uint32_t hash;
			Segment* segment;
			Bucket_t* homeBucket;
			uint32_t hopInfo;
			{
				uint32_t idxSegment;
				uint32_t idxBegBucket;

				hash = Hash()(key);
				idxSegment = hash & this->segmentMask;
				idxBegBucket = hash & this->bucketMask;
				segment = &this->segments[idxSegment];
				segment->lock();
				homeBucket = &this->buckets[idxBegBucket];
				hopInfo = homeBucket->getHopInfo();
			}

			// CHECK IF ALREADY CONTAIN ................
			uint8_t idx;
			Bucket_t* currBucket;
			while (hopInfo != 0U) {
				idx = findFirstSet(hopInfo);
				currBucket = homeBucket + idx;
				currBucket->lock();
				if (currBucket->equal(key, hash)) {
					homeBucket->offIndex(idx);
					currBucket->destruct();
					currBucket->unlock();
					segment->unlock();
					return true;
				}
				currBucket->unlock();
				hopInfo &= ~(1U << idx);
			}
			segment->unlock();
			return false;
		}

		bool contains(KType&& key) {
			return contains(key);
		}

		bool contains(const KType& key) {
			return get(key) != nullptr;
		}
	};
}

#endif // CLIB_CONCURRENCY_MAP_HPP
