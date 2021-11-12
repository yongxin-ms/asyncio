#pragma once
#include <vector>
#include <memory>
#include <asio.hpp>

namespace asyncio {

using IOContext = asio::io_context;
using IOWorker = asio::executor_work_guard<asio::io_context::executor_type>;

class ContextPool {
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
			this->m_contextThreads.emplace_back(
				std::make_unique<std::thread>([this, i]() { this->m_contexts[i]->run(); }));
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
