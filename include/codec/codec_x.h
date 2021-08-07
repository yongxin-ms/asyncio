#pragma once
#include "codec.h"

namespace asyncio {

class CodecX : public Codec {
public:
	virtual void Decode(const char* data, size_t len) = 0;
};

class CodecXProto {
public:
	virtual void OnMessage(uint32_t msg_id, std::shared_ptr<std::string> data) {

	}
}

}