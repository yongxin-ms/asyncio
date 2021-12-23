#pragma once
#include <asyncio/util.h>
#include <asyncio/log.h>
#include <asyncio/type.h>
#include <asyncio/protocol.h>
#include <asyncio/transport.h>
#include <asyncio/event_loop.h>
#include <asyncio/context_pool.h>
#include <asyncio/http_base.h>
#include <asyncio/codec/codec.h>
#include <asyncio/codec/codec_len.h>
#include <asyncio/codec/codec_user_header.h>

namespace asyncio {
inline void LogObjCounter() {
	ASYNCIO_LOG_INFO("DelayTimer count:%d/%d", asyncio::ObjCounter<asyncio::DelayTimer>::GetCount(),
		asyncio::ObjCounter<asyncio::DelayTimer>::GetMaxCount());

	ASYNCIO_LOG_INFO("Listener count:%d/%d", asyncio::ObjCounter<asyncio::Listener>::GetCount(),
		asyncio::ObjCounter<asyncio::Listener>::GetMaxCount());
	ASYNCIO_LOG_INFO("Transport count:%d/%d", asyncio::ObjCounter<asyncio::Transport>::GetCount(),
		asyncio::ObjCounter<asyncio::Transport>::GetMaxCount());
	ASYNCIO_LOG_INFO("Protocol count:%d/%d", asyncio::ObjCounter<asyncio::Protocol>::GetCount(),
		asyncio::ObjCounter<asyncio::Protocol>::GetMaxCount());

	ASYNCIO_LOG_INFO("http::server count:%d/%d", asyncio::ObjCounter<asyncio::http::server>::GetCount(),
		asyncio::ObjCounter<asyncio::http::server>::GetMaxCount());
	ASYNCIO_LOG_INFO("http::connection count:%d/%d", asyncio::ObjCounter<asyncio::http::connection>::GetCount(),
		asyncio::ObjCounter<asyncio::http::connection>::GetMaxCount());
}
} // namespace asyncio
