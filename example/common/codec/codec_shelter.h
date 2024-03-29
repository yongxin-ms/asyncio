﻿#pragma once
#include <functional>
#include <asyncio/codec/codec.h>
#include <asyncio/codec/bucket.h>
#include <asyncio/protocol.h>
#include <asyncio/log.h>

/*

tPacketHeader {
	uint16 datalen;	//除掉此报文头的报文体长度
	uint16 checksum;//除掉此报文头的报文体校验和
	uint8 flag;		//0
	int32 sequence;	//0
	int64 reserve;	//0
};

tPacketHeader header;
uint32 pnamelen;
char[] msg_name;
char[] msg;

*/

namespace asyncio {

class CodecShelter : public Codec {
	using USER_MSG_CALLBACK = std::function<void(const StringPtr&, const StringPtr&)>;

public:
	CodecShelter(USER_MSG_CALLBACK&& func,
		uint32_t rx_buffer_size = DEFAULT_RX_BUFFER_SIZE,
		uint32_t packet_size_limit = MAX_PACKET_SIZE)
		: Codec(rx_buffer_size, packet_size_limit)
		, m_user_msg_func(std::move(func)) {}

	virtual bool Decode(size_t len) override {
		// len是本次接收到的数据长度
		write_pos_ += len;					//需要更新一下最新的写入位置
		size_t left_len = GetRemainedLen(); //缓冲区内的数据总长度

		while (left_len > 0) {
			if (!bucket_.header.is_full() || !bucket_.msg_name_len.is_full()) {
				//
				// 一个完整包的长度最少是header.size() + sizeof(msg_name_len)
				//
				if (left_len < bucket_.header.size() + bucket_.msg_name_len.size()) {
					break;
				}

				if (bucket_.header.fill(read_pos_, left_len) && bucket_.msg_name_len.fill(read_pos_, left_len)) {
					if (IsOverSize(bucket_.msg_name_len.get())) {
						ASYNCIO_LOG_WARN(
							"Close transport because of packet length(%d) over limit", bucket_.msg_name_len.get());
						return false;
					}

					uint32_t data_len = bucket_.header.get().datalen;
					if (IsOverSize(data_len)) {
						ASYNCIO_LOG_WARN("Close transport because of packet length(%d) over limit", data_len);
						return false;
					}

					bucket_.msg_name.reset(bucket_.msg_name_len.get());
					bucket_.msg.reset(data_len - bucket_.msg_name_len.size() - bucket_.msg_name_len.get());
				}
			}

			bucket_.msg_name.fill(read_pos_, left_len);
			bucket_.msg.fill(read_pos_, left_len);

			// msg字段收完表示整个报文都已经完整了
			if (bucket_.msg.is_full()) {
				m_user_msg_func(bucket_.msg_name.get(), bucket_.msg.get());

				bucket_.header.reset();
				bucket_.msg_name_len.reset();
			}
		}

		ReArrangePos();
		return true;
	}

	StringPtr Encode(const std::string& msg_name, const std::string& msg) const {
		const size_t datalen = sizeof(uint32_t) + msg_name.size() + msg.size();
		auto p = std::make_shared<std::string>(TcpMsgHeader::size() + datalen, 0);
		size_t cur = 0;
		TcpMsgHeader* header = (TcpMsgHeader*)&p->at(cur);
		cur += TcpMsgHeader::size();

		uint32_t msg_name_len = uint32_t(msg_name.size());
		memcpy(&p->at(cur), &msg_name_len, sizeof(msg_name_len));
		cur += sizeof(msg_name_len);

		memcpy(&p->at(cur), msg_name.data(), msg_name.size());
		cur += msg_name.size();

		memcpy(&p->at(cur), msg.data(), msg.size());

		header->datalen = uint16_t(datalen);
		// header->checksum = ;
		return p;
	}

#pragma pack(push, 1)
	struct TcpMsgHeader {
		TcpMsgHeader() {
			memset(this, 0, size());
		}
		constexpr static size_t size() {
			return sizeof(TcpMsgHeader);
		}

		uint16_t datalen;  //除掉此报文头的报文体长度
		uint16_t checksum; //除掉此报文头的报文体校验和
		uint8_t flag;	   // 0
		int32_t sequence;  // 0
		int64_t reserve;   // 0
	};
#pragma pack(pop)

private:
	struct TcpMsgBucket {
		BucketPod<TcpMsgHeader> header;
		BucketPod<uint32_t> msg_name_len;
		BucketString msg_name;
		BucketString msg;
	};

private:
	USER_MSG_CALLBACK m_user_msg_func;
	TcpMsgBucket bucket_;
};

} // namespace asyncio
