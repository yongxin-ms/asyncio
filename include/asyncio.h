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
