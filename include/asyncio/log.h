/* asyncio implementation.
 *
 * Copyright (c) 2019-2022, Will <will at i007 dot cc>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of asyncio nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
