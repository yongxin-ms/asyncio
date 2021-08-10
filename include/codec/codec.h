#pragma once

namespace asyncio {

class Transport;
typedef std::shared_ptr<Transport> TransportPtr;

//
// 解码器
// 1.解决黏包问题
// 2.能使用较小的缓冲区接收较大的报文
//

class Codec {
public:
	enum {
		DEFAULT_RX_BUFFER_SIZE = 1024,
	};

	Codec(uint32_t packet_size_limit)
		: packet_size_limit_(packet_size_limit) {
		Reset();
	}

	void Reset(size_t size = DEFAULT_RX_BUFFER_SIZE) {
		if (rx_buf_.size() != size) {
			rx_buf_.resize(size);
		}
		read_pos_ = rx_buf_.data();
		write_pos_ = read_pos_;
	}

	std::pair<char*, size_t> GetRxBuffer() {
		return std::make_pair((char*)write_pos_, rx_buf_.size() - GetRemainedLen());
	}

	virtual void Decode(TransportPtr transport, size_t len) = 0;

protected:
	size_t GetRemainedLen() { return write_pos_ - read_pos_; }
	void ReArrangePos() {
		if (read_pos_ == rx_buf_.data())
			return;

		size_t remained_len = GetRemainedLen();
		if (remained_len == 0) {
			read_pos_ = rx_buf_.data();
			write_pos_ = rx_buf_.data();
		} else {
			memmove(&rx_buf_[0], read_pos_, remained_len);
			read_pos_ = rx_buf_.data();
			write_pos_ = read_pos_ + remained_len;
		}
	}

protected:
	const char* write_pos_ = nullptr;
	const char* read_pos_ = nullptr;
	std::string rx_buf_;
	uint32_t packet_size_limit_;
};

} // namespace asyncio
