#include "app.h"

App::App()
	: m_event_loop(4) {}

void App::Init(const std::string& ip, uint16_t port, int conn_count) {
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");
	ASYNCIO_LOG_INFO("+            GProtoTestClient Start           +");
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");

	asyncio::g_log->SetLogLevel(asyncio::Log::kDebug);
	m_conn_mgr.StartConnect(ip, port, conn_count);

	m_1second_timer = m_event_loop.CallLater(
		1000, [this]() { OnOneSecondTimer(); }, asyncio::DelayTimer::RUN_FOREVER);
}

void App::Run() {
	m_event_loop.RunForever();
	ASYNCIO_LOG_INFO("GProtoTestClient Stopped");
}

void App::Stop() {
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");
	ASYNCIO_LOG_INFO("+            GProtoTestClient Stop            +");
	ASYNCIO_LOG_INFO("+++++++++++++++++++++++++++++++++++++++++++++++");

	m_event_loop.Stop();
}

void App::IncQps() {
	++m_cur_qps;
}

void App::OnOneSecondTimer() {
	if (m_cur_qps != 0) {
		ASYNCIO_LOG_INFO("Cur qps:%d", m_cur_qps);
		m_cur_qps = 0;
	}
}
