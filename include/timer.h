#pragma once
#include <functional>
#include <asio.hpp>

namespace asyncio {
class TimerWrap {
	using MSG_CALLBACK = std::function<void()>;

public:
	TimerWrap(asio::io_context& context, int milliseconds, MSG_CALLBACK&& func)
		: m_context(context)
		, m_milliseconds(milliseconds)
		, m_func(std::move(func))
		, m_timer(context, std::chrono::milliseconds(milliseconds)) {
		m_timer.async_wait(std::bind(&TimerWrap::TimerFunc, this, std::placeholders::_1));
	}

	void Cancel() { m_timer.cancel(); }

private:
	void TimerFunc(std::error_code ec) { m_func(); }

private:
	asio::io_context& m_context;
	const int m_milliseconds;
	MSG_CALLBACK m_func;
	asio::steady_timer m_timer;
};

} // namespace asyncio
