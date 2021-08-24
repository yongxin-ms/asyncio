#pragma once
#include "codec.h"
#include "bucket.h"
#include "log.h"

namespace asyncio {

class CodecLen : public Codec {
public:
	using USER_MSG_CALLBACK = std::function<void(std::shared_ptr<std::string>)>;
	CodecLen(USER_MSG_CALLBACK&& func, uint32_t rx_buffer_size = DEFAULT_RX_BUFFER_SIZE,
			 uint32_t packet_size_limit = MAX_PACKET_SIZE)
		: Codec(rx_buffer_size, packet_size_limit)
		, m_user_msg_func(std::move(func)) {}
	virtual ~CodecLen() {}

	virtual void Reset() override {
		Codec::Reset();
		bucket_.head.reset();
	}

	virtual void Decode(TransportPtr transport, size_t len) override {
		// len是本次接收到的数据长度
		write_pos_ += len;					//需要更新一下最新的写入位置
		size_t left_len = GetRemainedLen(); //缓冲区内的数据总长度

		while (left_len > 0) {
			if (!bucket_.head.is_full()) {
				// 长度不够，等下一次
				if (left_len < bucket_.head.size())
					break;

				if (bucket_.head.fill(read_pos_, left_len)) {
					// 不允许报文长度为0，也不允许超长
					if (bucket_.head.get().len <= 0 || IsOverSize(bucket_.head.get().len)) {
						transport->Close(EC_PACKET_OVER_SIZE);
						ASYNCIO_LOG_WARN("Close transport because of packet length(%d) over limit",
										 bucket_.head.get().len);
						return;
					}

					bucket_.data.reset(bucket_.head.get().len);
				}
			}

			if (bucket_.data.is_full() || bucket_.data.fill(read_pos_, left_len)) {
				m_user_msg_func(bucket_.data.get());
				bucket_.head.reset();
			}
		}

		ReArrangePos();
	}

	std::shared_ptr<std::string> Encode(const char* buf, size_t len) {
		if (len <= 0)
			return nullptr;
		auto p = std::make_shared<std::string>(TcpMsgHeader::size() + len, 0);
		TcpMsgHeader* header = (TcpMsgHeader*)&p->at(0);
		header->len = (uint32_t)len;
		if (len > 0) {
			memcpy(&p->at(TcpMsgHeader::size()), buf, len);
		}
		return p;
	}

private:
	struct TcpMsgHeader {
		TcpMsgHeader() { len = 0; }
		constexpr static size_t size() { return sizeof(TcpMsgHeader); }

		uint32_t len; //这个长度是指报文体的长度，不包括此报文头的长度
	};

	struct TcpMsgBucket {
		BucketPod<TcpMsgHeader> head;
		BucketString data;
	};

private:
	USER_MSG_CALLBACK m_user_msg_func;
	TcpMsgBucket bucket_;
};

} // namespace asyncio
