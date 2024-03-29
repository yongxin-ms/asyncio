﻿#pragma once
#include <asyncio/codec/codec.h>
#include <asyncio/codec/bucket.h>
#include <asyncio/protocol.h>
#include <asyncio/log.h>

namespace asyncio {

class CodecTrans : public Codec {
	using USER_MSG_CALLBACK = std::function<void(uint64_t trans_id, uint32_t msg_id, const StringPtr&)>;

public:
	CodecTrans(USER_MSG_CALLBACK&& func,
		uint32_t rx_buffer_size = DEFAULT_RX_BUFFER_SIZE,
		uint32_t packet_size_limit = MAX_PACKET_SIZE)
		: Codec(rx_buffer_size, packet_size_limit)
		, m_user_msg_func(std::move(func)) {}

	virtual bool Decode(size_t len) override {
		// len是本次接收到的数据长度
		write_pos_ += len;					//需要更新一下最新的写入位置
		size_t left_len = GetRemainedLen(); //缓冲区内的数据总长度

		while (left_len > 0) {
			if (!bucket_.header.is_full()) {
				if (left_len < bucket_.header.size())
					break;

				if (bucket_.header.fill(read_pos_, left_len)) {
					uint32_t original_len = BigLittleSwap32(bucket_.header.get().len);
					if (IsOverSize(original_len)) {
						ASYNCIO_LOG_WARN("Close transport because of packet length(%d) over limit(%d)", original_len);
						return false;
					}

					bucket_.data.reset(original_len);
				}
			}

			if (bucket_.data.is_full() || bucket_.data.fill(read_pos_, left_len)) {
				uint32_t original_msgid = BigLittleSwap32(bucket_.header.get().msg_id);
				m_user_msg_func(bucket_.header.get().trans_sid, original_msgid, bucket_.data.get());
				bucket_.header.reset();
			}
		}

		ReArrangePos();
		return true;
	}

	StringPtr Encode(uint32_t msgID, uint64_t trans_sid, const char* buf, size_t len) const {
		auto p = std::make_shared<std::string>(TcpMsgHeader::size() + len, 0);
		TcpMsgHeader* header = (TcpMsgHeader*)&p->at(0);
		header->len = BigLittleSwap32(len);
		header->msg_id = BigLittleSwap32(msgID);
		header->trans_sid = trans_sid;
		if (len > 0) {
			memcpy(&p->at(TcpMsgHeader::size()), buf, len);
		}
		return p;
	}

private:
	struct TcpMsgHeader {
		TcpMsgHeader() {
			memset(this, 0, size());
		}
		constexpr static int size() {
			return sizeof(TcpMsgHeader);
		}

		uint32_t len; //这个长度是指报文体的长度，没有包括报文头的长度
		uint32_t msg_id;
		uint64_t trans_sid; //转发的SessionId
	};

	struct TcpMsgBucket {
		BucketPod<TcpMsgHeader> header;
		BucketString data;
	};

private:
	USER_MSG_CALLBACK m_user_msg_func;
	TcpMsgBucket bucket_;
};

} // namespace asyncio
