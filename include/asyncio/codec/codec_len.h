#pragma once
#include <asyncio/codec/codec.h>
#include <asyncio/codec/bucket.h>
#include <asyncio/log.h>

namespace asyncio {

class CodecLen final : public Codec {
public:
	using USER_MSG_CALLBACK = std::function<void(const StringPtr&)>;

	CodecLen(USER_MSG_CALLBACK&& func,
		uint32_t rx_buffer_size = DEFAULT_RX_BUFFER_SIZE,
		uint32_t packet_size_limit = MAX_PACKET_SIZE,
		bool small_endian = true)
		: Codec(rx_buffer_size, packet_size_limit)
		, m_user_msg_func(std::move(func))
		, m_small_endian(small_endian) {}

	virtual bool Decode(size_t len) override {
		write_pos_ += len;
		size_t left_len = GetRemainedLen();

		while (left_len > 0) {
			if (!bucket_.head.is_full()) {
				if (left_len < bucket_.head.size())
					break;

				if (bucket_.head.fill(read_pos_, left_len)) {
					uint32_t body_len = bucket_.head.get().len;

					if (!m_small_endian)
						BigLittleSwap32(body_len);

					if (body_len <= 0 || IsOverSize(body_len)) {
						ASYNCIO_LOG_WARN("Close transport because of packet length(%d) over limit", body_len);
						return false;
					}

					bucket_.data.reset(body_len);
				}
			}

			if (bucket_.data.is_full() || bucket_.data.fill(read_pos_, left_len)) {
				m_user_msg_func(bucket_.data.get());
				bucket_.head.reset();
			}
		}

		ReArrangePos();
		return true;
	}

	StringPtr Encode(const char* buf, size_t len) const {
		if (len <= 0)
			return nullptr;
		auto p = std::make_shared<std::string>(TcpMsgHeader::size() + len, 0);
		TcpMsgHeader* header = (TcpMsgHeader*)&p->at(0);
		header->len = (uint32_t)len;
		if (!m_small_endian)
			BigLittleSwap32(header->len);
		if (len > 0) {
			memcpy(&p->at(TcpMsgHeader::size()), buf, len);
		}
		return p;
	}

private:
	struct TcpMsgHeader {
		TcpMsgHeader() {
			len = 0;
		}
		constexpr static size_t size() {
			return sizeof(TcpMsgHeader);
		}

		uint32_t len;
	};

	struct TcpMsgBucket {
		BucketPod<TcpMsgHeader> head;
		BucketString data;
	};

private:
	USER_MSG_CALLBACK m_user_msg_func;
	TcpMsgBucket bucket_;
	const bool m_small_endian;
};

} // namespace asyncio
