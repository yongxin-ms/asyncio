#pragma once
#include <functional>
#include <asyncio/codec/codec.h>
#include <asyncio/codec/bucket.h>
#include <asyncio/transport.h>
#include <asyncio/log.h>

namespace asyncio {

class CodecGProto : public Codec {
public:
	using USER_MSG_CALLBACK = std::function<void(uint32_t msg_id, const StringPtr&)>;
	using PONG_CALLBACK = std::function<void()>;

	CodecGProto(USER_MSG_CALLBACK&& msg_func, PONG_CALLBACK&& pong_func,
				uint32_t rx_buffer_size = DEFAULT_RX_BUFFER_SIZE, uint32_t packet_size_limit = MAX_PACKET_SIZE)
		: Codec(rx_buffer_size, packet_size_limit)
		, m_user_msg_func(std::move(msg_func))
		, m_pong_func(std::move(pong_func)) {}

	void Init(const TransportPtr& transport) {
		Reset();
		bucket_.header.reset();
		bucket_.msg_id.reset();
		m_transport = transport;
	}

	virtual void Decode(size_t len) override {
		// len是本次接收到的数据长度
		write_pos_ += len;					//需要更新一下最新的写入位置
		size_t left_len = GetRemainedLen(); //缓冲区内的数据总长度

		while (left_len > 0) {
			if (!bucket_.header.is_full()) {
				if (left_len < bucket_.header.size())
					break;

				if (bucket_.header.fill(read_pos_, left_len)) {
					auto ctrl = bucket_.header.get().ctrl;
					if (ctrl == CTL_DATA) {
						if (bucket_.header.get().len < bucket_.msg_id.size()) {
							ASYNCIO_LOG_WARN("Close transport because of packet length(%d)", bucket_.header.get().len);
							m_transport->Close(EC_PACKET_OVER_SIZE);
							return;
						}

						bucket_.msg_id.reset();
						uint32_t original_len = bucket_.header.get().len - sizeof(uint32_t);
						if (IsOverSize(original_len)) {
							ASYNCIO_LOG_WARN("Close transport because of packet length(%d) over limit", original_len);
							m_transport->Close(EC_PACKET_OVER_SIZE);
							return;
						}

						bucket_.data.reset(original_len);
					} else if (ctrl == CTL_PING) {
						if (bucket_.header.get().len != 0) {
							ASYNCIO_LOG_WARN("Close transport because of packet wrong len:%d",
											 bucket_.header.get().len);
							m_transport->Close(EC_KICK);
							return;
						}

						ASYNCIO_LOG_DEBUG("received ping");
						send_pong(); // 反射一个pong
						bucket_.header.reset();
						bucket_.msg_id.reset();
						continue;
					} else if (ctrl == CTL_PONG) {
						if (bucket_.header.get().len != 0) {
							ASYNCIO_LOG_WARN("Close transport because of packet wrong len:%d",
											 bucket_.header.get().len);
							m_transport->Close(EC_KICK);
							return;
						}

						ASYNCIO_LOG_DEBUG("received pong");
						m_pong_func();
						bucket_.header.reset();
						bucket_.msg_id.reset();
						continue;
					} else if (ctrl == CTL_CLOSE) {
						ASYNCIO_LOG_DEBUG("received close");
						//对方要求关闭连接
						m_transport->Close(EC_SHUT_DOWN);
						return;
					}
				}
			}

			if (!bucket_.msg_id.is_full()) {
				if (left_len < bucket_.msg_id.size())
					break;
				bucket_.msg_id.fill(read_pos_, left_len);
			}

			bucket_.data.fill(read_pos_, left_len);

			// data长度可以为0
			if (bucket_.data.is_full()) {
				m_user_msg_func(bucket_.msg_id.get(), bucket_.data.get());
				bucket_.header.reset();
				bucket_.msg_id.reset();
			}
		}

		ReArrangePos();
	}

	StringPtr Encode(uint32_t msgID, const char* buf, size_t len) const {
		const size_t body_len = sizeof(msgID) + len;
		auto p = std::make_shared<std::string>(TcpMsgHeader::size() + body_len, 0);
		TcpMsgHeader* header = (TcpMsgHeader*)&p->at(0);
		header->len = body_len;
		header->ctrl = CTL_DATA;
		header->enc = ecnryptWord_;
		memcpy(&p->at(TcpMsgHeader::size()), &msgID, sizeof(msgID));
		if (len > 0) {
			memcpy(&p->at(TcpMsgHeader::size() + sizeof(msgID)), buf, len);
		}
		return p;
	}

	void send_ping() {
		ASYNCIO_LOG_DEBUG("send ping");

		TcpMsgHeader ping;
		ping.len = 0;
		ping.ctrl = CTL_PING;
		ping.enc = ecnryptWord_;
		auto data = std::make_shared<std::string>();
		data->append((const char*)&ping, sizeof(ping));
		m_transport->Write(data);
	}

	void send_pong() {
		ASYNCIO_LOG_DEBUG("send pong");

		TcpMsgHeader pong;
		pong.len = 0;
		pong.ctrl = CTL_PONG;
		pong.enc = ecnryptWord_;
		auto data = std::make_shared<std::string>();
		data->append((const char*)&pong, sizeof(pong));
		m_transport->Write(data);
	}

private:
	enum TcpControl {
		CTL_DATA = 0,
		CTL_PING,
		CTL_PONG,
		CTL_CLOSE,
	};

	struct TcpMsgHeader {
		TcpMsgHeader() { memset(this, 0, size()); }
		constexpr static size_t size() { return sizeof(TcpMsgHeader); }

		uint32_t len : 24; //这个长度是指报文体的长度，不包括此报文头的长度
		uint32_t enc : 6;
		uint32_t ctrl : 2;
	};

	struct TcpMsgBucket {
		BucketPod<TcpMsgHeader> header;
		BucketPod<uint32_t> msg_id;
		BucketString data;
	};

private:
	USER_MSG_CALLBACK m_user_msg_func;
	PONG_CALLBACK m_pong_func;
	uint8_t ecnryptWord_ = 0;
	TcpMsgBucket bucket_;
	TransportPtr m_transport;
};

} // namespace asyncio
