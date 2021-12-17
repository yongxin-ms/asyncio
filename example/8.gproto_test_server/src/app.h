#pragma once
#include "session_mgr.h"
#include "singleton.h"
#include "id_worker.h"

class App : public Singleton<App> {
public:
	App();

	bool Init(uint16_t port);
	void Run();
	void Stop();

	uint64_t CreateId() {
		return m_idwork.CreateId();
	}

	asyncio::EventLoop& GetEventLoop() {
		return m_event_loop;
	}

	void IncQps();

private:
	void OnOneSecondTimer();
	void OnOneMinuteTimer();

private:
	asyncio::EventLoop m_event_loop;
	id_worker::IdWorker m_idwork;
	MySessionMgr m_session_mgr;
	asyncio::DelayTimerPtr m_1second_timer;
	asyncio::DelayTimerPtr m_1minute_timer;
	int m_cur_qps = 0;
};

#define g_EventLoop App::Instance()->GetEventLoop()
