#pragma once
#include <string>
#include <list>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <assert.h>
#include <time.h>
#include <cstring>
#include "util.h"

namespace asyncio {

class CronExpression {
public:
	enum DATA_TYPE {
		DT_SECOND = 0,
		DT_MINUTE = 1,
		DT_HOUR = 2,
		DT_DAY_OF_MONTH = 3,
		DT_MONTH = 4,
		DT_YEAR = 5,
		DT_MAX,
	};

	// 获得数值枚举
	static bool GetValues(const std::string& input, DATA_TYPE data_type, std::vector<int>& values) {

		//
		// 注意：枚举之前是','，为了能在csv中使用改成了';'
		// 20181226 xinyong
		//

		static const char CRON_SEPERATOR_ENUM = ';';
		static const char CRON_SEPERATOR_RANGE = '-';
		static const char CRON_SEPERATOR_INTERVAL = '/';

		if (input == "*") {
			auto pair_range = GetRangeFromType(data_type);
			for (auto i = pair_range.first; i <= pair_range.second; ++i) {
				values.push_back(i);
			}
		} else if (input.find_first_of(CRON_SEPERATOR_ENUM) != std::string::npos) {
			//枚举
			std::vector<int> v;
			util::Text::SplitInt(v, input, CRON_SEPERATOR_ENUM);
			std::pair<int, int> pair_range = GetRangeFromType(data_type);
			for (auto value : v) {
				if (value < pair_range.first || value > pair_range.second) {
					return false;
				}
				values.push_back(value);
			}
		} else if (input.find_first_of(CRON_SEPERATOR_RANGE) != std::string::npos) {
			//范围
			std::vector<int> v;
			util::Text::SplitInt(v, input, CRON_SEPERATOR_RANGE);
			if (v.size() != 2) {
				return false;
			}

			int from = v[0];
			int to = v[1];
			std::pair<int, int> pair_range = GetRangeFromType(data_type);
			if (from < pair_range.first || to > pair_range.second) {
				return false;
			}

			for (int i = from; i <= to; i++) {
				values.push_back(i);
			}
		} else if (input.find_first_of(CRON_SEPERATOR_INTERVAL) != std::string::npos) {
			//间隔
			std::vector<int> v;
			util::Text::SplitInt(v, input, CRON_SEPERATOR_INTERVAL);
			if (v.size() != 2) {
				return false;
			}

			int from = v[0];
			int interval = v[1];
			std::pair<int, int> pair_range = GetRangeFromType(data_type);
			if (from < pair_range.first || interval < 0) {
				return false;
			}

			for (int i = from; i <= pair_range.second; i += interval) {
				values.push_back(i);
			}
		} else {
			//具体数值
			std::pair<int, int> pair_range = GetRangeFromType(data_type);
			int value = atoi(input.data());
			if (value < pair_range.first || value > pair_range.second) {
				return false;
			}

			values.push_back(value);
		}

		assert(values.size() > 0);
		return values.size() > 0;
	}

private:
	// 获得范围
	static std::pair<int, int> GetRangeFromType(DATA_TYPE data_type) {
		int from = 0;
		int to = 0;

		switch (data_type) {
		case CronExpression::DT_SECOND:
		case CronExpression::DT_MINUTE:
			from = 0;
			to = 59;
			break;
		case CronExpression::DT_HOUR:
			from = 0;
			to = 23;
			break;
		case CronExpression::DT_DAY_OF_MONTH:
			from = 1;
			to = 31;
			break;
		case CronExpression::DT_MONTH:
			from = 1;
			to = 12;
			break;
		case CronExpression::DT_YEAR:
			from = 1970;
			to = 2099;
			break;
		case CronExpression::DT_MAX:
			assert(false);
			break;
		}

		return std::make_pair(from, to);
	}
};

struct CronWheel {
	CronWheel()
		: cur_index(0) {}

	//返回值：是否有进位
	bool init(int init_value) {
		for (size_t i = cur_index; i < values.size(); ++i) {
			if (values[i] >= init_value) {
				cur_index = i;
				return false;
			}
		}

		cur_index = 0;
		return true;
	}

	size_t cur_index;
	std::vector<int> values;
};

class TimerMgr;
using FUNC_CALLBACK = std::function<void()>;

class BaseTimer : public std::enable_shared_from_this<BaseTimer> {
	friend class TimerMgr;

public:
	BaseTimer(TimerMgr& owner, FUNC_CALLBACK&& func)
		: owner_(owner)
		, func_(std::move(func))
		, is_in_list_(false) {}

	inline void Cancel();

protected:
	std::list<std::shared_ptr<BaseTimer>>::iterator& GetIt() { return it_; }
	void SetIt(const std::list<std::shared_ptr<BaseTimer>>::iterator& it) { it_ = it; }
	bool GetIsInList() const { return is_in_list_; }
	void SetIsInList(bool b) { is_in_list_ = b; }

	virtual void DoFunc() = 0;
	virtual std::chrono::system_clock::time_point GetCurTime() const = 0;

protected:
	TimerMgr& owner_;
	FUNC_CALLBACK func_;
	std::list<std::shared_ptr<BaseTimer>>::iterator it_;
	bool is_in_list_;
};

class CronTimer : public BaseTimer {
	friend class TimerMgr;

public:
	CronTimer(TimerMgr& owner, std::vector<CronWheel>&& wheels, FUNC_CALLBACK&& func, int count)
		: BaseTimer(owner, std::move(func))
		, wheels_(std::move(wheels))
		, over_flowed_(false)
		, count_left_(count) {
		tm local_tm;
		time_t time_now = time(nullptr);

#ifdef _WIN32
		localtime_s(&local_tm, &time_now);
#else
		localtime_r(&time_now, &local_tm);
#endif // _WIN32

		std::vector<int> init_values;
		init_values.push_back(local_tm.tm_sec);
		init_values.push_back(local_tm.tm_min);
		init_values.push_back(local_tm.tm_hour);
		init_values.push_back(local_tm.tm_mday);
		init_values.push_back(local_tm.tm_mon + 1);
		init_values.push_back(local_tm.tm_year + 1900);

		bool addup = false;
		for (int i = 0; i < CronExpression::DT_MAX; i++) {
			auto& wheel = wheels_[i];
			auto init_value = addup ? init_values[i] + 1 : init_values[i];
			addup = wheel.init(init_value);
		}
	}

	virtual ~CronTimer() {}
	inline void DoFunc() override;
	std::chrono::system_clock::time_point GetCurTime() const override {
		tm next_tm;
		memset(&next_tm, 0, sizeof(next_tm));
		next_tm.tm_sec = GetCurValue(CronExpression::DT_SECOND);
		next_tm.tm_min = GetCurValue(CronExpression::DT_MINUTE);
		next_tm.tm_hour = GetCurValue(CronExpression::DT_HOUR);
		next_tm.tm_mday = GetCurValue(CronExpression::DT_DAY_OF_MONTH);
		next_tm.tm_mon = GetCurValue(CronExpression::DT_MONTH) - 1;
		next_tm.tm_year = GetCurValue(CronExpression::DT_YEAR) - 1900;

		return std::chrono::system_clock::from_time_t(mktime(&next_tm));
	}

private:
	// 前进到下一格
	void Next(int data_type) {
		if (data_type >= CronExpression::DT_MAX) {
			// 溢出了表明此定时器已经失效，不应该再被执行
			over_flowed_ = true;
			return;
		}

		auto& wheel = wheels_[data_type];
		if (wheel.cur_index == wheel.values.size() - 1) {
			wheel.cur_index = 0;
			Next(data_type + 1);
		} else {
			++wheel.cur_index;
		}
	}

	int GetCurValue(int data_type) const {
		const auto& wheel = wheels_[data_type];
		return wheel.values[wheel.cur_index];
	}

private:
	std::vector<CronWheel> wheels_;
	bool over_flowed_;
	int count_left_;
};

class LaterTimer : public BaseTimer {
	friend class TimerMgr;

public:
	LaterTimer(TimerMgr& owner, int milliseconds, FUNC_CALLBACK&& func, int count)
		: BaseTimer(owner, std::move(func))
		, mill_seconds_(milliseconds)
		, count_left_(count) {
		cur_time_ = std::chrono::system_clock::now();
		Next();
	}

	virtual ~LaterTimer() {}
	inline void DoFunc() override;
	std::chrono::system_clock::time_point GetCurTime() const override { return cur_time_; }

private:
	//前进到下一格
	void Next() {
		auto time_now = std::chrono::system_clock::now();
		while (true) {
			cur_time_ += std::chrono::milliseconds(mill_seconds_);
			if (cur_time_ > time_now) {
				break;
			}
		}
	}

private:
	const int mill_seconds_;
	std::chrono::system_clock::time_point cur_time_;
	int count_left_;
};

class TimerMgr {
	friend class BaseTimer;
	friend class CronTimer;
	friend class LaterTimer;

public:
	TimerMgr() {}
	TimerMgr(const TimerMgr&) = delete;
	const TimerMgr& operator=(const TimerMgr&) = delete;

	void Stop() {
		timers_.clear();
		stopped_ = true;
	}

	enum {
		RUN_FOREVER = 0,
	};

	// 新增一个Cron表达式的定时器，缺省永远执行
	std::shared_ptr<BaseTimer> AddTimer(
		const std::string& timer_string, FUNC_CALLBACK&& func, int count = RUN_FOREVER) {
		if (stopped_) {
			return nullptr;
		}

		std::vector<std::string> v;
		util::Text::SplitStr(v, timer_string, ' ');
		if (v.size() != CronExpression::DT_MAX) {
			assert(false);
			return nullptr;
		}

		std::vector<CronWheel> wheels;
		for (int i = 0; i < CronExpression::DT_MAX; i++) {
			const auto& expression = v[i];
			CronExpression::DATA_TYPE data_type = CronExpression::DATA_TYPE(i);
			CronWheel wheel;
			if (!CronExpression::GetValues(expression, data_type, wheel.values)) {
				assert(false);
				return nullptr;
			}

			wheels.emplace_back(wheel);
		}

		auto p = std::make_shared<CronTimer>(*this, std::move(wheels), std::move(func), count);
		insert(p);
		return p;
	}

	// 新增一个延时执行的定时器，缺省运行一次
	std::shared_ptr<BaseTimer> AddDelayTimer(int milliseconds, FUNC_CALLBACK&& func, int count = 1) {
		if (stopped_) {
			return nullptr;
		}

		assert(milliseconds > 0);
		milliseconds = (std::max)(milliseconds, 1);
		auto p = std::make_shared<LaterTimer>(*this, milliseconds, std::move(func), count);
		insert(p);
		return p;
	}

	std::chrono::system_clock::time_point GetNearestTime() {
		auto it = timers_.begin();
		if (it == timers_.end()) {
			return (std::chrono::system_clock::time_point::max)();
		} else {
			return it->first;
		}
	}

	size_t Update() {
		auto time_now = std::chrono::system_clock::now();
		size_t count = 0;

		for (auto it = timers_.begin(); it != timers_.end();) {
			auto expire_time = it->first;
			auto& timer_list = it->second;
			if (expire_time > time_now) {
				break;
			}

			while (!timer_list.empty()) {
				auto p = *timer_list.begin();
				p->DoFunc();
				++count;
			}

			it = timers_.erase(it);
		}

		return count;
	}

private:
	void insert(const std::shared_ptr<BaseTimer>& p) {
		assert(!p->GetIsInList());

		auto t = p->GetCurTime();
		auto it = timers_.find(t);
		if (it == timers_.end()) {
			std::list<std::shared_ptr<BaseTimer>> l;
			timers_.insert(std::make_pair(t, l));
			it = timers_.find(t);
		}

		auto& l = it->second;
		p->SetIt(l.insert(l.end(), p));
		p->SetIsInList(true);
	}

	bool remove(const std::shared_ptr<BaseTimer>& p) {
		assert(p->GetIsInList());

		auto t = p->GetCurTime();
		auto it = timers_.find(t);
		if (it == timers_.end()) {
			assert(false);
			return false;
		}

		auto& l = it->second;
		if (p->GetIt() == l.end()) {
			assert(false);
			return false;
		}

		l.erase(p->GetIt());
		p->SetIt(l.end());
		p->SetIsInList(false);
		return true;
	}

private:
	std::map<std::chrono::system_clock::time_point, std::list<std::shared_ptr<BaseTimer>>> timers_;
	bool stopped_ = false;
};

void BaseTimer::Cancel() {
	if (!GetIsInList()) {
		return;
	}

	auto self = shared_from_this();
	owner_.remove(self);
}

void CronTimer::DoFunc() {
	func_();

	// 可能用户在定时器中取消了自己
	if (GetIsInList()) {
		auto self = shared_from_this();
		owner_.remove(self);

		Next(CronExpression::DT_SECOND);
		if ((count_left_ <= TimerMgr::RUN_FOREVER || --count_left_ > 0) && !over_flowed_) {
			owner_.insert(self);
		}
	}
}

void LaterTimer::DoFunc() {
	func_();

	// 可能用户在定时器中取消了自己
	if (GetIsInList()) {
		auto self = shared_from_this();
		owner_.remove(self);

		if (count_left_ <= TimerMgr::RUN_FOREVER || --count_left_ > 0) {
			Next();
			owner_.insert(self);
		}
	}
}

} // namespace asyncio
