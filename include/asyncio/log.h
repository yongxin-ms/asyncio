#pragma once
#include <functional>
#include <cstdarg>

#define ASYNCIO_LOG_DEBUG(fmt, ...)                                                                                    \
	if (asyncio::g_log != nullptr)                                                                                     \
		asyncio::g_log->DoLog(asyncio::Log::kDebug, "[%s:%d %s()] " fmt, asyncio::Log::GetFileName(__FILE__),          \
			__LINE__, __func__, ##__VA_ARGS__);
#define ASYNCIO_LOG_INFO(fmt, ...)                                                                                     \
	if (asyncio::g_log != nullptr)                                                                                     \
		asyncio::g_log->DoLog(asyncio::Log::kInfo, "[%s:%d %s()] " fmt, asyncio::Log::GetFileName(__FILE__), __LINE__, \
			__func__, ##__VA_ARGS__);
#define ASYNCIO_LOG_WARN(fmt, ...)                                                                                     \
	if (asyncio::g_log != nullptr)                                                                                     \
		asyncio::g_log->DoLog(asyncio::Log::kWarning, "[%s:%d %s()] " fmt, asyncio::Log::GetFileName(__FILE__),        \
			__LINE__, __func__, ##__VA_ARGS__);
#define ASYNCIO_LOG_ERROR(fmt, ...)                                                                                    \
	if (asyncio::g_log != nullptr)                                                                                     \
		asyncio::g_log->DoLog(asyncio::Log::kError, "[%s:%d %s()] " fmt, asyncio::Log::GetFileName(__FILE__),          \
			__LINE__, __func__, ##__VA_ARGS__);

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
		, m_log_level(lv) {}

	void SetLogLevel(LogLevel lv) {
		m_log_level = lv;
	}

	void DoLog(LogLevel lv, const char* fmt, ...) {
		if (lv <= m_log_level) {
			char buf[1024];
			va_list args;
			va_start(args, fmt);
			std::vsnprintf(buf, sizeof(buf) - 1, fmt, args);
			va_end(args);
			m_log_func(lv, buf);
		}
	}

	static const char* GetFileName(const char* file) {
		auto i = strrchr(file, '/');
		if (i != nullptr)
			return i + 1;
		else
			return file;
	}

private:
	LOG_FUNC m_log_func;
	LogLevel m_log_level = kDebug;
};

static Log* g_log = nullptr;

static void SetLogHandler(Log::LOG_FUNC&& func, Log::LogLevel log_level) {
	if (g_log != nullptr) {
		delete g_log;
		g_log = nullptr;
	}

	g_log = new Log(std::move(func), log_level);
}

} // namespace asyncio
