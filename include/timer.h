#pragma once
#include <functional>
#include <asio.hpp>

namespace asyncio {
class DelayTimer : public std::enable_shared_from_this<DelayTimer> {
public:
	using FUNC_CALLBACK = std::function<void()>;

	DelayTimer(asio::io_context& context, int milliseconds, FUNC_CALLBACK&& func)
		: m_milliseconds(milliseconds)
		, m_func(std::move(func))
		, m_timer(context) {
	}
	~DelayTimer() { Cancel(); }
	void Cancel() { m_timer.cancel(); }

	void Start() {
		auto self = shared_from_this();
		m_timer.expires_after(std::chrono::milliseconds(m_milliseconds));
		m_timer.async_wait([self](std::error_code ec) {
			if (ec.value() == 0) {
				self->m_func();
			}
		});
	}

private:
	const int m_milliseconds;
	FUNC_CALLBACK m_func;
	asio::steady_timer m_timer;
};

using DelayTimerPtr = std::shared_ptr<asyncio::DelayTimer>;

} // namespace asyncio
