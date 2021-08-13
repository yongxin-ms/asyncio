#include "app.h"

App::App()
	: my_event_loop(4)
	, m_idwork(1, 1, [](const char* s) { ASYNCIO_LOG_DEBUG("{}", s); })
	, m_session_mgr(my_event_loop, m_idwork) {}

bool App::Init(uint16_t port) {
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");
	ASYNCIO_LOG_INFO("+            GProtoTestServer Start           +");
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");

	asyncio::g_log->SetLogLevel(asyncio::Log::kDebug);

	if (!m_session_mgr.Init(port))
		return false;

	m_timer = my_event_loop.CallLater(1000, [this]() {
		OnOnSecondTimer();
		m_timer->Start();
	});

	return true;
}

void App::Run() {
	my_event_loop.RunForever();
	ASYNCIO_LOG_INFO("GProtoTestServer Stopped");
}

void App::Stop() {
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");
	ASYNCIO_LOG_INFO("+            GProtoTestServer Stop            +");
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");
	my_event_loop.Stop();
}

void App::IncQps() {
	++m_cur_qps;
}

void App::OnOnSecondTimer() {
	if (m_cur_qps != 0) {
		ASYNCIO_LOG_INFO("Cur qps:%d, ConnCount:%llu", m_cur_qps, m_session_mgr.size());
		m_cur_qps = 0;
	}
}
