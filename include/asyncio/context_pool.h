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
#include <vector>
#include <memory>
#include <asio.hpp>
#include <asyncio/log.h>
#include <asyncio/obj_counter.h>

namespace asyncio {

using IOContext = asio::io_context;
using IOWorker = asio::executor_work_guard<asio::io_context::executor_type>;

class ContextPool : public ObjCounter<ContextPool> {
public:
	explicit ContextPool(size_t pool_size)
		: m_nextContextIndex(0) {
		if (pool_size == 0)
			throw std::runtime_error("asio::io_context size is 0");

		for (size_t i = 0; i < pool_size; ++i) {
			auto context = std::make_unique<IOContext>();
			auto io_worker = asio::make_work_guard(*context);
			m_workers.emplace_back(std::move(io_worker));
			m_contexts.emplace_back(std::move(context));
		}

		for (size_t i = 0; i < pool_size; ++i) {
			m_contextThreads.emplace_back(std::make_unique<std::thread>([this, i]() {
				auto thread_id = std::this_thread::get_id();
				ASYNCIO_LOG_INFO("ContextPool thread:%d, thread_id:%u", i, thread_id);

				//
				m_contexts[i]->run();
			}));
		}
	}

	ContextPool(const ContextPool&) = delete;
	const ContextPool& operator=(const ContextPool&) = delete;
	~ContextPool() {
		for (auto& i : m_contexts) {
			i->stop();
		}

		for (auto& i : m_contextThreads) {
			i->join();
		}

		m_contextThreads.clear();
		m_workers.clear();
		m_contexts.clear();

		ASYNCIO_LOG_DEBUG("ContextPool destroyed");
	}

	IOContext& NextContext() {
		if (m_contexts.size() == 0) {
			throw std::runtime_error("forgot to call Init?");
		}

		auto& context = m_contexts[m_nextContextIndex];
		m_nextContextIndex = (m_nextContextIndex + 1) % m_contexts.size();
		return *context;
	}

private:
	std::vector<IOWorker> m_workers;
	std::vector<std::unique_ptr<std::thread>> m_contextThreads;
	std::vector<std::unique_ptr<IOContext>> m_contexts;
	std::atomic<uint32_t> m_nextContextIndex;
};
} // namespace asyncio
