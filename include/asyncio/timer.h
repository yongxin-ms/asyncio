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

	DelayTimer(std::thread::id thread_id, IOContext& context, int milliseconds, FUNC_CALLBACK&& func, int run_times)
		: m_thread_id(thread_id)
		, m_milliseconds(milliseconds)
		, m_timer(context)
		, m_expire(std::chrono::steady_clock::now())
		, m_running(false) {
		Run(std::move(func), run_times);
	}

	~DelayTimer() {
		ASYNCIO_LOG_DEBUG("DelayTimer destroyed");
		Cancel();
	}

	DelayTimer(const DelayTimer&) = delete;
	DelayTimer& operator=(const DelayTimer&) = delete;

private:
	void Cancel() {
		if (m_running) {
			m_running = false;

			auto cur_thread_id = std::this_thread::get_id();
			if (cur_thread_id != m_thread_id) {
				ASYNCIO_LOG_ERROR("Thread Error, cur_thread_id:%d, m_thread_id:%d", cur_thread_id, m_thread_id);
				throw std::runtime_error("this function can only be called in main loop thread");
			}
			m_timer.cancel();
		}
	}

	void Run(FUNC_CALLBACK&& func, int run_times) {
		m_expire += std::chrono::milliseconds(m_milliseconds);
		m_timer.expires_at(m_expire);

		m_running = true;
		m_timer.async_wait([func = std::move(func), this, run_times](std::error_code ec) mutable -> void {
			if (!ec) {
				auto func_(func);
				if (run_times == RUN_FOREVER) {
					Run(std::move(func), run_times);
				} else if (run_times >= 2) {
					Run(std::move(func), run_times - 1);
				} else {
					Cancel();
				}

				func_();
			} else if (ec != asio::error::operation_aborted) {
				ASYNCIO_LOG_ERROR("DelayTimer async_wait ec:%d err_msg:%s", ec.value(), ec.message().data());
			}
		});
	}

private:
	const std::thread::id m_thread_id;
	const int m_milliseconds;
	asio::steady_timer m_timer;
	std::chrono::steady_clock::time_point m_expire;
	bool m_running;
};

} // namespace asyncio
