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
