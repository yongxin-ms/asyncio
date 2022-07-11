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
#include <memory>
#include <asyncio/type.h>
#include <asyncio/obj_counter.h>

namespace asyncio {

class Protocol : public ObjCounter<Protocol> {
public:
	Protocol() = default;
	virtual ~Protocol() = default;
	Protocol(const Protocol&) = delete;
	Protocol& operator=(const Protocol&) = delete;

	virtual std::pair<char*, size_t> GetRxBuffer() = 0;
	virtual void ConnectionMade(const TransportPtr& transport) = 0;
	virtual void ConnectionLost(const TransportPtr& transport, int err_code) = 0;
	virtual bool DataReceived(size_t len) = 0;
	virtual size_t Write(const StringPtr& s) = 0;
};

class ProtocolFactory {
public:
	ProtocolFactory() = default;
	virtual ~ProtocolFactory() = default;
	virtual ProtocolPtr CreateProtocol() = 0;

	ProtocolFactory(const ProtocolFactory&) = delete;
	ProtocolFactory& operator=(const ProtocolFactory&) = delete;
};

} // namespace asyncio
