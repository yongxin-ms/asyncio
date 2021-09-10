#pragma once
#include <asyncio.h>
#include "conn_mgr.h"
#include "singleton.h"

class App : public Singleton<App> {
public:
	App();

	void Init(const std::string& ip, uint16_t port, int conn_count);
	void Run();
	void Stop();

	void IncQps();

private:
	void OnOneSecondTimer();

private:
	asyncio::EventLoop my_event_loop;
	MyConnMgr m_conn_mgr;
	asyncio::DelayTimerPtr m_1second_timer;
	int m_cur_qps = 0;
};
