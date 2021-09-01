#pragma once
#include "asyncio.h"
#include "session_mgr.h"
#include "singleton.h"
#include "id_worker.h"

class App : public Singleton<App> {
public:
	App();

	bool Init(uint16_t port);
	void Run();
	void Stop();
	
	void IncQps();

private:
	void OnOneSecondTimer();

private:
	asyncio::EventLoop my_event_loop;
	id_worker::IdWorker m_idwork;
	MySessionMgr m_session_mgr;
	asyncio::DelayTimerPtr m_1second_timer;
	int m_cur_qps = 0;
};
