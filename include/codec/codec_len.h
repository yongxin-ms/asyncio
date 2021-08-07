#pragma once

namespace asyncio {

class CodecLen : public Codec {
public:
	virtual void Decode(const char* data, size_t len) override {

	}

	void Encode() {}
};

}