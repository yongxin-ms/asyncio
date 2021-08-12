#pragma once
#include <mutex>
#include <chrono>
#include <sstream>
#include <functional>
#include <thread>

/**
 * @brief 分布式id生成类
 * SnowFlake的优点: 整体上按照时间自增排序,
 * 并且整个分布式系统内不会产生ID碰撞(由数据中心ID和机器ID作区分), 并且效率较高, 经测试,
 * SnowFlake每秒能够产生400万左右的Id.
 */

namespace id_worker {

class IdWorker {
public:
	IdWorker(uint32_t datacenterId, uint32_t workerId, std::function<void(const char*)> f) {
		id_.nId = 0;
		id_.stId.workerId = workerId;
		id_.stId.datacenterId = datacenterId;
		log_func_ = f;
	}

	IdWorker(const IdWorker&) = delete;
	const IdWorker& operator=(const IdWorker&) = delete;

	/**
	 * 生成一个ID (该方法是线程安全的)
	 *
	 * @return SnowflakeId
	 */
	uint64_t CreateId() {
		auto timestamp = GetCurMilliSeconds();
		std::lock_guard<std::mutex> lock{mutex_};

		// 如果当前时间小于上一次ID生成的时间戳，说明系统时钟回退过
		if (timestamp < lastTimestamp_) {
			auto offset = lastTimestamp_ - timestamp;
			if (offset <= 10) {
				// 等待两倍时间，让时间追上来
				std::this_thread::sleep_for(std::chrono::milliseconds(offset * 2));
				timestamp = GetCurMilliSeconds();
				if (log_func_ != nullptr) {
					std::ostringstream text;
					text << "[IdWorker] "
						 << "sleep for timer rollback, offset:" << offset << "ms";
					log_func_(text.str().c_str());
				}
			} else {
				id_.stId.timeRollback++;
				if (log_func_ != nullptr) {
					std::ostringstream text;
					text << "[IdWorker] "
						 << "increase counter for timer rollback, offset:" << offset << "ms"
						 << ",counter:" << id_.stId.timeRollback;
					log_func_(text.str().c_str());
				}
			}
		}

		if (lastTimestamp_ == timestamp) {
			// 如果是同一时间生成的，则进行毫秒内序列
			id_.stId.sequence++;
			if (0 == id_.stId.sequence) {
				// 毫秒内序列溢出, 阻塞到下一个毫秒,获得新的时间戳
				timestamp = WaitTillNextMilliSeconds(lastTimestamp_);
			}
		} else {
			id_.stId.sequence = 0;
		}

		lastTimestamp_ = timestamp;
		id_.stId.timestamp = timestamp - START_TIME;
		return id_.nId;
	}

private:
	/**
	 * 返回以毫秒为单位的当前时间
	 *
	 * @return 当前时间(毫秒)
	 */
	int64_t GetCurMilliSeconds() const {
		auto t = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now());
		return t.time_since_epoch().count();
	}

	/**
	 * 阻塞到下一个毫秒，直到获得新的时间戳
	 *
	 * @param lastTimestamp 上次生成ID的时间截
	 * @return 当前时间戳
	 */
	int64_t WaitTillNextMilliSeconds(int64_t lastTimestamp) const {
		auto timestamp = GetCurMilliSeconds();
		while (timestamp <= lastTimestamp) {
			timestamp = GetCurMilliSeconds();
		}
		return timestamp;
	}

private:
	struct StId {
		uint64_t sequence : 10;
		uint64_t workerId : 5;
		uint64_t datacenterId : 5;
		uint64_t timeRollback : 5; //时间回拨计数
		uint64_t timestamp : 38;
		uint64_t unused : 1;
	};

	union UnionId {
		StId stId;
		uint64_t nId;
	};

	const int64_t START_TIME = 1514736000000; //开始时间截 (2018-01-01 00:00:00.000)

	std::mutex mutex_;
	int64_t lastTimestamp_ = 0;
	UnionId id_;
	std::function<void(const char*)> log_func_ = nullptr;
};
} // namespace id_worker
