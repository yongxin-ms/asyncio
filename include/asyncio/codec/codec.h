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
		/*
		 * 缺省缓冲区大小，空间换时间，缓冲区越大，接收超长报文的性能越好
		*/
		DEFAULT_RX_BUFFER_SIZE = 1024,

		/*
		 * 最大报文长度，超过此长度的报文会认为是非法的，解码器会断开连接，否则可能会把内存吃光造成崩溃
		 */
		MAX_PACKET_SIZE = 10 * 1024 * 1024,
	};

	Codec(uint32_t rx_buffer_size, uint32_t packet_size_limit)
		: rx_buffer_size_(rx_buffer_size)
		, packet_size_limit_(packet_size_limit) {
		Reset();
	}

	virtual ~Codec() = default;

	virtual void Reset() {
		if (rx_buf_.size() != rx_buffer_size_) {
			rx_buf_.resize(rx_buffer_size_);
		}
		read_pos_ = rx_buf_.data();
		write_pos_ = read_pos_;
	}

	std::pair<char*, size_t> GetRxBuffer() const {
		return std::make_pair((char*)write_pos_, rx_buf_.size() - GetRemainedLen());
	}

	/*
	 * 如果在解码运行在io线程中， transport有可能被主线程清空，所以这里要判断transport的合法性
	 */
	virtual void Decode(TransportPtr transport, size_t len) = 0;

protected:
	size_t GetRemainedLen() const { return write_pos_ - read_pos_; }
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

	bool IsOverSize(uint32_t len) const { return packet_size_limit_ > 0 && len > packet_size_limit_; }

protected:
	const char* write_pos_ = nullptr;
	const char* read_pos_ = nullptr;

private:
	std::string rx_buf_;
	uint32_t rx_buffer_size_;
	uint32_t packet_size_limit_;
};

} // namespace asyncio
