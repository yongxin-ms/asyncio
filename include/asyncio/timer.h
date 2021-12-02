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

	DelayTimer(std::thread::id loop_thread_id, IOContext& context, int milliseconds, FUNC_CALLBACK&& func)
		: m_thread_id(loop_thread_id)
		, m_milliseconds(milliseconds)
		, m_func(std::move(func))
		, m_timer(context)
		, m_running(false) {}

	~DelayTimer() {
		ASYNCIO_LOG_DEBUG("DelayTimer destroyed milliseconds:%d, times_left:%d", m_milliseconds, m_run_times_left);
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

		if (m_running) {
			m_timer.cancel();
			m_running = false;
		}
	}

	void Run(int run_times = RUN_ONCE) {
		auto cur_thread_id = std::this_thread::get_id();
		if (cur_thread_id != m_thread_id) {
			ASYNCIO_LOG_ERROR("Thread Error, cur_thread_id:%d, m_thread_id:%d", cur_thread_id, m_thread_id);
			throw std::runtime_error("this function can only be called in main loop thread");
		}

		if (run_times < 0) {
			throw std::runtime_error("wrong left_times");
		}

		m_run_times_left = run_times;
		Cancel();
		m_running = true;
		m_timer.expires_after(std::chrono::milliseconds(m_milliseconds));
		m_timer.async_wait([this](std::error_code ec) {
			if (!ec) {
				m_running = false;
				if (m_run_times_left == RUN_FOREVER || --m_run_times_left > 0) {
					Run(m_run_times_left);
				}

				m_func();
			} else {
				//ASYNCIO_LOG_ERROR("DelayTimer m_timer.async_wait err_msg:%s", ec.message().data());
			}
		});
	}

private:
	const std::thread::id m_thread_id;
	const int m_milliseconds;
	FUNC_CALLBACK m_func;
	asio::steady_timer m_timer;
	bool m_running;
	int m_run_times_left;
};

} // namespace asyncio
