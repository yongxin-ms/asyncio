#pragma once
#include <asio.hpp>
#include <asyncio/log.h>

namespace asyncio {
class DelayTimer {
public:
	using FUNC_CALLBACK = std::function<void()>;
	enum {
		RUN_ONCE = 1,	 // 运行一次
		RUN_FOREVER = 0, // 永远运行
	};

	DelayTimer(std::thread::id thread_id, IOContext& context, int milliseconds, FUNC_CALLBACK&& func,
		int run_times = RUN_ONCE)
		: m_thread_id(thread_id)
		, m_timer(context) {
		Run(milliseconds, std::move(func), run_times);
	}

	~DelayTimer() {
		ASYNCIO_LOG_DEBUG("DelayTimer destroyed");
		Cancel();
	}

	DelayTimer(const DelayTimer&) = delete;
	DelayTimer& operator=(const DelayTimer&) = delete;

	void Cancel() {
		auto cur_thread_id = std::this_thread::get_id();
		if (cur_thread_id != m_thread_id) {
			ASYNCIO_LOG_ERROR("Thread Error, cur_thread_id:%d, m_thread_id:%d", cur_thread_id, m_thread_id);
			throw std::runtime_error("this function can only be called in main loop thread");
		}
		m_timer.cancel();
	}

private:
	void Run(int milliseconds, FUNC_CALLBACK&& func, int run_times) {
		m_timer.expires_after(std::chrono::milliseconds(milliseconds));
		m_timer.async_wait([func = std::move(func), this, run_times, milliseconds](std::error_code ec) mutable -> void {
			if (!ec) {
				func();
				if (run_times == RUN_FOREVER) {
					Run(milliseconds, std::move(func), run_times);
				} else if (run_times > 0) {
					Run(milliseconds, std::move(func), run_times - 1);
				}
			} else if (ec != asio::error::operation_aborted) {
				ASYNCIO_LOG_ERROR("DelayTimer async_wait ec:%d err_msg:%s", ec.value(), ec.message().data());
			}
		});
	}

private:
	const std::thread::id m_thread_id;
	asio::steady_timer m_timer;
};

} // namespace asyncio
