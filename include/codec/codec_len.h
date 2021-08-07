#pragma once
#include "codec.h"

namespace asyncio {

class CodecLen : public Codec {
	using USER_MSG_CALLBACK = std::function<void(std::shared_ptr<std::string>)>;

public:
	CodecLen(USER_MSG_CALLBACK&& func)
		: m_user_msg_func(std::move(func)) {
	}

	virtual void Decode(TransportPtr transport, const char* data, size_t len) override {
	}

	std::shared_ptr<std::string> Encode(const char* data, size_t len) {
	}

private:
	USER_MSG_CALLBACK m_user_msg_func;
};

} // namespace asyncio
