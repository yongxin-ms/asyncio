#pragma once

namespace asyncio {

class Codec {
public:
	enum {
		DEFAULT_RX_BUFFER_SIZE = 64 * 1024,
		MAX_PACKET_SIZE = 10 * 1024 * 1024,
	};

	Codec(uint32_t rx_buffer_size, uint32_t packet_size_limit)
		: rx_buffer_size_(rx_buffer_size)
		, packet_size_limit_(packet_size_limit) {
		Reset();
	}

	virtual ~Codec() = default;
	Codec(const Codec&) = delete;
	Codec& operator=(const Codec&) = delete;

	std::pair<char*, size_t> GetRxBuffer() const {
		return std::make_pair((char*)write_pos_, rx_buf_.size() - GetRemainedLen());
	}

	virtual bool Decode(size_t len) = 0;

protected:
	void Reset() {
		if (rx_buf_.size() != rx_buffer_size_) {
			rx_buf_.resize(rx_buffer_size_);
		}
		read_pos_ = rx_buf_.data();
		write_pos_ = read_pos_;
	}

	size_t GetRemainedLen() const {
		return write_pos_ - read_pos_;
	}

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

	bool IsOverSize(uint32_t len) const {
		return packet_size_limit_ > 0 && len > packet_size_limit_;
	}

private:
	std::string rx_buf_;
	uint32_t rx_buffer_size_;

protected:
	const char* write_pos_ = nullptr;
	const char* read_pos_ = nullptr;
	uint32_t packet_size_limit_;
};

} // namespace asyncio
