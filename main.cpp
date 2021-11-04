#include <chrono>
#include <vector>
#include <random>
#include <iostream>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

#include "clib/util.hpp"
#include "clib/signal.h"
#include "clib/log/logger.h"
#include "clib/threadpool.h"
#include "clib/database/connector.h"
#include "clib/map/hashf.hpp"
#include "clib/map/concurrency_map.hpp"

#define N_ITEMS 26870U
#define N_TIMES 1U
#define N_LIMIT_FIND 20000U

clib::ConcurrencyMap<int, int> map1{};
std::unordered_map<int32_t, int32_t> map2{};

auto range1 = N_ITEMS / 4;
auto range2 = 2 * range1;
auto range3 = 3 * range1;
auto range4 = N_ITEMS;

bool dbDone{ false };
std::vector<int> data(N_ITEMS, 0);
std::shared_mutex smutex{};

alignas(64) std::atomic<uint32_t> countExecTimes{ 0U };
alignas(64) std::atomic<uint32_t> countFindTimes{ 0U };
alignas(64) std::atomic<uint32_t> countRemoveTimes{ 0U };
alignas(64) std::atomic<uint32_t> countInsertTimes{ 0U };
alignas(64) std::atomic<uint32_t> count{ 0U };

alignas(64) std::atomic<uint64_t> timestamp10{ 0U };
alignas(64) std::atomic<uint64_t> timestamp11{ 0U };
alignas(64) std::atomic<uint64_t> timestamp12{ 0U };
alignas(64) std::atomic<uint64_t> timestamp20{ 0U };
alignas(64) std::atomic<uint64_t> timestamp21{ 0U };
alignas(64) std::atomic<uint64_t> timestamp22{ 0U };
alignas(64) std::atomic<uint64_t> timestamp30{ 0U };
alignas(64) std::atomic<uint64_t> timestamp31{ 0U };
alignas(64) std::atomic<uint64_t> timestamp32{ 0U };
alignas(64) std::atomic<uint64_t> timestamp40{ 0U };
alignas(64) std::atomic<uint64_t> timestamp41{ 0U };
alignas(64) std::atomic<uint64_t> timestamp42{ 0U };

int main() {
	// INIT ................
	clib::Signal::Instance.init();
	clib::Threadpool::Instance.init();
	clib::log::Logger::Instance.init("./test");
	
	// TEST SIGNAL ................
	auto [err, _] = clib::Signal::Instance.subscribe(Type::SIG_INT, Order::TAIL, [](int signalNumber) {
		clib::Threadpool::Instance.exit();
		clib::log::Logger::Instance.log(FATAL, "Received signal interupt");
		clib::log::Logger::Instance.logf(FATAL, "Received signal interupt");
		clib::log::Logger::Instance.exit();
	});
	if (!err.nil()) {
		clib::log::Logger::Instance.log(ERROR, err.toString());
	}

	// TEST DATABASE ................
	printf("TEST DATABASE ................\n");
	clib::db::Connector connector{ clib::db::Config{ "tcp://127.0.0.1", "root", "1234", "test", 3306, 10 } };
	clib::Threadpool::Instance.push([&connector] {
		connector.run([](clib::db::Connection& connection)->clib::Error {
			connection.beginTask();
			auto error = connection.transactionQuery("CALL method(?)", "row0", [](clib::db::Result&& res)->clib::Error {
				auto rows = res.get();
				while (rows->next()) {
					printf("%d-%s\n", rows->getInt(1), rows->getString(2).c_str());
				}
				return clib::Error{};
			});
			connection.endTask();
			return error;
		});
		dbDone = true;
	});
	while (!dbDone) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	// TEST CONCURRENCY MAP ................
	uint32_t nFind{ 0U };
	uint32_t nRemove{ 0U };
	uint32_t nInsert{ 0U };
	{
		for (auto i = 0; i < N_ITEMS; ++i) {
			std::random_device dev;
			std::mt19937 rng(dev());
			std::uniform_int_distribution<std::mt19937::result_type> dist6(0, INT32_MAX);
			auto x = dist6(rng);
			if (x % 2 == 0) {
				data[i] = 0;
				nRemove++;
			}
			else {
				data[i] = 1;
				nFind++;
			}
		}

		std::unordered_set<int> set;
		for (auto i = 0; i < range1;) {
			std::random_device dev;
			std::mt19937 rng(dev());
			std::uniform_int_distribution<std::mt19937::result_type> dist6(0, N_ITEMS - 1);
			auto x = dist6(rng);
			auto size = set.size();
			set.insert(x);
			if (size < set.size()) {
				if (data[x] == 0) {
					nRemove--;
				}
				else {
					nFind--;
				}
				data[x] = 2;
				nInsert++;
				i++;
			}
		}
	}
	printf("Number of remove: %d\n", nRemove);
	printf("Number of find: %d\n", nFind);
	printf("Number of insert: %d\n", nInsert);

	map1.init(N_ITEMS);
	for (auto i = 0; i < N_ITEMS; ++i) {
		if (data[i] != 2) {
			map1.set(i, std::move(i));
		}
	}


	for (auto i = 0; i < N_TIMES; ++i) {
		clib::Threadpool::Instance.push([] {
			std::chrono::steady_clock::time_point beg;
			std::chrono::steady_clock::time_point end;
			uint32_t i;
			while (true) {
				i = count.fetch_add(1, std::memory_order_acquire);
				if (i >= N_ITEMS) {
					break;
				}
				if (data[i] == 0) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.remove(i)) {
						countRemoveTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp10 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 1) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.contains(i)) {
						countFindTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp11 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 2) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.set(i, i)) {
						countInsertTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp12 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
			}
			printf("Done 1.1\n");
			countExecTimes++;
		});
		clib::Threadpool::Instance.push([] {
			std::chrono::steady_clock::time_point beg;
			std::chrono::steady_clock::time_point end;
			uint32_t i;
			while (true) {
				i = count.fetch_add(1, std::memory_order_acquire);
				if (i >= N_ITEMS) {
					break;
				}
				if (data[i] == 0) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.remove(i)) {
						countRemoveTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp20 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 1) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.contains(i)) {
						countFindTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp21 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 2) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.set(i, i)) {
						countInsertTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp22 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
			}
			printf("Done 1.2\n");
			countExecTimes++;
		});
		clib::Threadpool::Instance.push([] {
			std::chrono::steady_clock::time_point beg;
			std::chrono::steady_clock::time_point end;
			uint32_t i;
			while (true) {
				i = count.fetch_add(1, std::memory_order_acquire);
				if (i >= N_ITEMS) {
					break;
				}
				if (data[i] == 0) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.remove(i)) {
						countRemoveTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp30 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 1) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.contains(i)) {
						countFindTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp31 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 2) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.set(i, i)) {
						countInsertTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp32 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
			}
			printf("Done 1.3\n");
			countExecTimes++;
		});
		clib::Threadpool::Instance.push([] {
			std::chrono::steady_clock::time_point beg;
			std::chrono::steady_clock::time_point end;
			uint32_t i;
			while (true) {
				i = count.fetch_add(1, std::memory_order_acquire);
				if (i >= N_ITEMS) {
					break;
				}
				if (data[i] == 0) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.remove(i)) {
						countRemoveTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp40 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 1) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.contains(i)) {
						countFindTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp41 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 2) {
					beg = std::chrono::high_resolution_clock::now();
					if (map1.set(i, i)) {
						countInsertTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp42 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
			}
			printf("Done 1.4\n");
			countExecTimes++;
		});
	}

	while (countExecTimes.load(std::memory_order_acquire) != N_TIMES * 4) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	printf("TEST CON-MAP1 ................\n");
	printf("AVG time of map-1: remove: %f (micro-secs)\n\t\t   find: %f (micro-secs)\n\t\t   insert: %f (micro-secs)\n", 
		   1.0f * (timestamp10 + timestamp20 + timestamp30 + timestamp40) / N_TIMES, 
		   1.0f * (timestamp11 + timestamp21 + timestamp31 + timestamp41) / N_TIMES,
		   1.0f * (timestamp12 + timestamp22 + timestamp32 + timestamp42) / N_TIMES
	);
	printf("Remove times: %d\n", countRemoveTimes.load());
	printf("Find times: %d\n", countFindTimes.load());
	printf("Insert times: %d\n", countInsertTimes.load());

	timestamp10.store(0, std::memory_order_release);
	timestamp11.store(0, std::memory_order_release);
	timestamp12.store(0, std::memory_order_release);
	timestamp20.store(0, std::memory_order_release);
	timestamp21.store(0, std::memory_order_release);
	timestamp22.store(0, std::memory_order_release);
	timestamp30.store(0, std::memory_order_release);
	timestamp31.store(0, std::memory_order_release);
	timestamp32.store(0, std::memory_order_release);
	timestamp40.store(0, std::memory_order_release);
	timestamp41.store(0, std::memory_order_release);
	timestamp42.store(0, std::memory_order_release);

	count.store(0, std::memory_order_release);
	countExecTimes.store(0, std::memory_order_release);
	countFindTimes.store(0, std::memory_order_release);
	countRemoveTimes.store(0, std::memory_order_release);
	countInsertTimes.store(0, std::memory_order_release);

	for (auto i = 0; i < N_ITEMS; ++i) {
		map2.emplace(i, i);
	}

	for (auto i = 0; i < N_TIMES; ++i) {
		clib::Threadpool::Instance.push([] {
			std::chrono::steady_clock::time_point beg;
			std::chrono::steady_clock::time_point end;
			uint32_t i;
			while (true) {
				i = count.fetch_add(1, std::memory_order_acquire);
				if (i >= N_ITEMS) {
					break;
				}
				if (data[i] == 0) {
					beg = std::chrono::high_resolution_clock::now();
					std::unique_lock lock{ smutex };
					if (map2.erase(i) > 0) {
						++countRemoveTimes;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp10 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 1) {
					beg = std::chrono::high_resolution_clock::now();
					std::shared_lock lock{ smutex };
					if (map2.contains(i)) {
						countFindTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp11 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 2) {
					beg = std::chrono::high_resolution_clock::now();
					std::unique_lock lock{ smutex };
					map2.emplace(i, i);
					countInsertTimes++;
					end = std::chrono::high_resolution_clock::now();
					timestamp12 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
			}
			printf("Done 2.1\n");
			countExecTimes++;
		});
		clib::Threadpool::Instance.push([] {
			std::chrono::steady_clock::time_point beg;
			std::chrono::steady_clock::time_point end;
			uint32_t i;
			while (true) {
				i = count.fetch_add(1, std::memory_order_acquire);
				if (i >= N_ITEMS) {
					break;
				}
				if (data[i] == 0) {
					beg = std::chrono::high_resolution_clock::now();
					std::unique_lock lock{ smutex };
					if (map2.erase(i) > 0) {
						++countRemoveTimes;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp20 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 1) {
					beg = std::chrono::high_resolution_clock::now();
					std::shared_lock lock{ smutex };
					if (map2.contains(i)) {
						countFindTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp21 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 2) {
					beg = std::chrono::high_resolution_clock::now();
					std::unique_lock lock{ smutex };
					map2.emplace(i, i);
					countInsertTimes++;
					end = std::chrono::high_resolution_clock::now();
					timestamp22 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
			}
			printf("Done 2.2\n");
			countExecTimes++;
		});
		clib::Threadpool::Instance.push([] {
			std::chrono::steady_clock::time_point beg;
			std::chrono::steady_clock::time_point end;
			uint32_t i;
			while (true) {
				i = count.fetch_add(1, std::memory_order_acquire);
				if (i >= N_ITEMS) {
					break;
				}
				if (data[i] == 0) {
					beg = std::chrono::high_resolution_clock::now();
					std::unique_lock lock{ smutex };
					if (map2.erase(i) > 0) {
						++countRemoveTimes;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp30 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 1) {
					beg = std::chrono::high_resolution_clock::now();
					std::shared_lock lock{ smutex };
					if (map2.contains(i)) {
						countFindTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp31 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 2) {
					beg = std::chrono::high_resolution_clock::now();
					std::unique_lock lock{ smutex };
					map2.emplace(i, i);
					countInsertTimes++;
					end = std::chrono::high_resolution_clock::now();
					timestamp32 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
			}
			printf("Done 2.3\n");
			countExecTimes++;
		});
		clib::Threadpool::Instance.push([] {
			std::chrono::steady_clock::time_point beg;
			std::chrono::steady_clock::time_point end;
			uint32_t i;
			while (true) {
				i = count.fetch_add(1, std::memory_order_acquire);
				if (i >= N_ITEMS) {
					break;
				}
				if (data[i] == 0) {
					beg = std::chrono::high_resolution_clock::now();
					std::unique_lock lock{ smutex };
					if (map2.erase(i) > 0) {
						++countRemoveTimes;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp40 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 1) {
					beg = std::chrono::high_resolution_clock::now();
					std::shared_lock lock{ smutex };
					if (map2.contains(i)) {
						countFindTimes++;
					}
					end = std::chrono::high_resolution_clock::now();
					timestamp41 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
				else if (data[i] == 2) {
					beg = std::chrono::high_resolution_clock::now();
					std::unique_lock lock{ smutex };
					map2.emplace(i, i);
					countInsertTimes++;
					end = std::chrono::high_resolution_clock::now();
					timestamp42 += std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count();
				}
			}
			printf("Done 2.4\n");
			countExecTimes++;
		});
	}

	while (countExecTimes.load(std::memory_order_acquire) != N_TIMES * 4) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	printf("TEST CON-MAP2-SMUTEX ................\n");
	printf("AVG time of map-1: remove: %f (micro-secs)\n\t\t   find: %f (micro-secs)\n\t\t   insert: %f (micro-secs)\n",
		1.0f * (timestamp10 + timestamp20 + timestamp30 + timestamp40) / N_TIMES,
		1.0f * (timestamp11 + timestamp21 + timestamp31 + timestamp41) / N_TIMES,
		1.0f * (timestamp12 + timestamp22 + timestamp32 + timestamp42) / N_TIMES
	);
	printf("Remove times: %d\n", countRemoveTimes.load());
	printf("Find times: %d\n", countFindTimes.load());
	printf("Insert times: %d\n", countInsertTimes.load());

	// TEST AGAIN ..................................
	std::vector<int> v1;
	std::vector<int> v2;

	for (auto i = 0; i < N_ITEMS; ++i) {
		if (data[i] == 1) {
			v1.push_back(i);
			auto res = map1.get(i);
			if (res != nullptr) {
				v2.push_back(*res.get());
			}
			else {
				v2.push_back(-1);
			}
		}
		else if (data[i] == 0) {
			auto res = map1.get(i);
			if (res != nullptr) {
				printf("%d\n", *res.get());
			}
		}
	}
	for (auto i = 0; i < nFind; ++i) {
		if (v1[i] != v2[i]) {
			printf("%d\n", v2[i]);
		}
	}


	// EXIT .......................................
	clib::Threadpool::Instance.exit();
	clib::log::Logger::Instance.exit();
	_ = getchar();
	return 0;
}