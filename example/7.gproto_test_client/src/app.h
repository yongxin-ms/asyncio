#pragma once
#include "conn_mgr.h"
#include "singleton.h"

class App : public Singleton<App> {
public:
	App();

	void Init(const std::string& ip, uint16_t port, int conn_count);
	void Run();
	void Stop();

	asyncio::EventLoop& GetEventLoop() {
		return m_event_loop;
	}

	void IncQps();

private:
	void OnOneSecondTimer();

private:
	asyncio::EventLoop m_event_loop;
	MyConnMgr m_conn_mgr;
	asyncio::DelayTimerPtr m_1second_timer;
	int m_cur_qps = 0;
};

#define g_EventLoop App::Instance()->GetEventLoop()
