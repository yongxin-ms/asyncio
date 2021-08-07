#pragma once
#include <functional>

namespace asyncio {

class Log {
public:
	enum LogLevel {
		kError = 1,
		kWarning,
		kInfo,
		kDebug,
	};

	using LOG_FUNC = std::function<void(LogLevel, const char*)>;

	Log(LOG_FUNC&& func, LogLevel lv) 
		: m_log_func(std::move(func))
		, m_log_level(lv) {
	}

	void SetLogLevel(LogLevel lv) {
		m_log_level = lv;
	}

	void DoLog(LogLevel lv, const char* fmt, ...) {
		if (lv <= m_log_level) {
			char buf[1024];
			va_list args;
			va_start(args, fmt);
			vsnprintf(buf, sizeof(buf) - 1, fmt, args);
			va_end(args);
			log_func_(lv, buf);
		}
	}

private:
	LOG_FUNC m_log_func;
	LogLevel m_log_level = kDebug;
};

}