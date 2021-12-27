#pragma once
#include <asio.hpp>
#include <asyncio/log.h>
#include <asyncio/obj_counter.h>

namespace asyncio {
enum {
	RUN_ONCE = 1,	 // 运行一次
	RUN_FOREVER = 0, // 永远运行
};

class DelayTimer : public ObjCounter<DelayTimer> {
public:
	using FUNC_CALLBACK = std::function<void()>;

	template <class Rep, class Period>
	DelayTimer(std::thread::id thread_id, IOContext& context, const std::chrono::duration<Rep, Period>& timeout,
		FUNC_CALLBACK&& func, int run_times)
		: m_thread_id(thread_id)
		, m_timer(context)
		, m_expire(std::chrono::steady_clock::now())
		, m_running(false) {
		Run(timeout, std::move(func), run_times);
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
				ASYNCIO_LOG_ERROR("Thread Error, cur_thread_id:%u, m_thread_id:%u", cur_thread_id, m_thread_id);
				throw std::runtime_error("this function can only be called in main loop thread");
			}
			m_timer.cancel();
		}
	}

	template <class Rep, class Period>
	void Run(const std::chrono::duration<Rep, Period>& timeout, FUNC_CALLBACK&& func, int run_times) {
		m_expire += timeout;
		m_timer.expires_at(m_expire);

		m_running = true;
		m_timer.async_wait([timeout, func = std::move(func), this, run_times](std::error_code ec) mutable -> void {
			if (!ec) {
				auto func_(func);
				if (run_times == RUN_FOREVER) {
					Run(timeout, std::move(func), run_times);
				} else if (run_times >= 2) {
					Run(timeout, std::move(func), run_times - 1);
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
	asio::steady_timer m_timer;
	std::chrono::steady_clock::time_point m_expire;
	bool m_running;
};

} // namespace asyncio
