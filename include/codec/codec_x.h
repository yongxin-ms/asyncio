#pragma once
#include "codec.h"

namespace asyncio {

class CodecX : public Codec {
	using USER_MSG_CALLBACK = std::function<void(uint32_t msg_id, std::shared_ptr<std::string>)>;

public:
	CodecX(USER_MSG_CALLBACK&& func)
		: m_user_msg_func(std::move(func)) {
	}

	virtual void Decode(TransportPtr transport, const char* data, size_t len) override {
	}

	std::shared_ptr<std::string> Encode(uint32_t msg_id, const char* data, size_t len) {
	}

private:
	USER_MSG_CALLBACK m_user_msg_func;
};

class CodecXProto {
public:
	virtual void OnMessage(uint32_t msg_id, std::shared_ptr<std::string> data) {
	}
};

} // namespace asyncio
