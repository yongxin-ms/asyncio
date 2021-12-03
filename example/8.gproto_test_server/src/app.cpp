#include "app.h"

App::App()
	: m_idwork(1, 1, [](const char* s) {
		ASYNCIO_LOG_DEBUG("{}", s);
	}) {}

bool App::Init(uint16_t port) {
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");
	ASYNCIO_LOG_INFO("+            GProtoTestServer Start           +");
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");

	asyncio::g_log->SetLogLevel(asyncio::Log::kDebug);

	if (!m_session_mgr.Init(port))
		return false;

	m_1second_timer = m_event_loop.CallLater(
		1000,
		[this]() {
			OnOneSecondTimer();
		},
		asyncio::DelayTimer::RUN_FOREVER);

	return true;
}

void App::Run() {
	m_event_loop.RunForever();
	ASYNCIO_LOG_INFO("GProtoTestServer Stopped");
}

void App::Stop() {
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");
	ASYNCIO_LOG_INFO("+            GProtoTestServer Stop            +");
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");
	m_event_loop.Stop();
}

void App::IncQps() {
	++m_cur_qps;
}

void App::OnOneSecondTimer() {
	if (m_cur_qps != 0) {
		ASYNCIO_LOG_INFO("Cur qps:%d, ConnCount:%llu", m_cur_qps, m_session_mgr.size());
		m_cur_qps = 0;
	}
}
