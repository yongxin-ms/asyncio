#pragma once
#include "codec.h"

namespace asyncio {

class CodecLen : public Codec {
public:
	virtual void Decode(TransportPtr transport, const char* data, size_t len) override {

	}

	std::shared_ptr<std::string> Encode(const char* data, size_t len) {
		
	}
};

}