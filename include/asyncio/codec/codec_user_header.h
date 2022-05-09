#pragma once
#include <functional>
#include <asyncio/codec/codec.h>
#include <asyncio/codec/bucket.h>

namespace asyncio {

template <typename UserHeader, uint32_t MAGIC_NUM>
class CodecUserHeader final : public Codec {
	struct TcpMsgHeader {
		TcpMsgHeader()
			: magic_num(0)
			, body_len(0) {}
		constexpr static size_t size() {
			return sizeof(TcpMsgHeader);
		}

		uint32_t magic_num;
		uint32_t body_len; // only body length, not including header
		UserHeader user_header;
	};

	using USER_MSG_CALLBACK = std::function<void(const UserHeader&, const StringPtr&)>;

public:
	CodecUserHeader(USER_MSG_CALLBACK&& func,
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
				if (left_len < bucket_.header.size()) {
					break;
				}

				if (bucket_.header.fill(read_pos_, left_len)) {
					auto body_len = bucket_.header.get().body_len;
					if (IsOverSize(body_len)) {
						ASYNCIO_LOG_WARN("Close transport because of packet length(%d) over limit(%d)", body_len);
						return false;
					}

					auto magic_num = bucket_.header.get().magic_num;
					if (magic_num != MAGIC_NUM) {
						ASYNCIO_LOG_WARN("Close transport because of wrong magic_num:0x%x", magic_num);
						return false;
					}

					bucket_.data.reset(body_len);
				}
			}

			if (bucket_.data.is_full() || bucket_.data.fill(read_pos_, left_len)) {
				m_user_msg_func(bucket_.header.get().user_header, bucket_.data.get());
				bucket_.header.reset();
			}
		}

		ReArrangePos();
		return true;
	}

	StringPtr Encode(const UserHeader& user_header, const char* buf, uint32_t len) const {
		auto p = std::make_shared<std::string>(TcpMsgHeader::size() + len, 0);
		TcpMsgHeader* header = (TcpMsgHeader*)&p->at(0);
		header->magic_num = MAGIC_NUM;
		memcpy(&header->user_header, &user_header, sizeof(UserHeader));
		header->body_len = len;
		if (len > 0) {
			memcpy(&p->at(TcpMsgHeader::size()), buf, len);
		}
		return p;
	}

private:
	struct TcpMsgBucket {
		BucketPod<TcpMsgHeader> header;
		BucketString data;
	};

private:
	USER_MSG_CALLBACK m_user_msg_func;
	TcpMsgBucket bucket_;
};

} // namespace asyncio
