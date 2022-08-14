#pragma once
#include <memory>
#include <string>
#include <algorithm>

typedef unsigned short int uint16;
typedef unsigned long int uint32;

#define BigLittleSwap16(A) ((((uint16)(A)&0xff00) >> 8) | (((uint16)(A)&0x00ff) << 8))
#define BigLittleSwap32(A)                                                                                             \
	((((uint32_t)(A)&0xff000000) >> 24) | (((uint32_t)(A)&0x00ff0000) >> 8) | (((uint32_t)(A)&0x0000ff00) << 8) |      \
		(((uint32_t)(A)&0x000000ff) << 24))

template <class T>
class BucketPod {
public:
	BucketPod()
		: m_is_full(false) {}

	void reset() {
		m_is_full = false;
	}

	const T& get() const {
		return m_t;
	}

	size_t size() const {
		return sizeof(m_t);
	}

	size_t filled_size() const {
		return m_is_full ? size() : 0;
	}

	bool is_full() const {
		return m_is_full;
	}

	bool fill(const char*& buf, size_t& len) {
		if (m_is_full)
			return false;

		size_t need = size();
		if (len < need)
			return false;

		memcpy(&m_t, buf, need);
		buf += need;
		len -= need;
		m_is_full = true;
		return true;
	}

private:
	bool m_is_full;
	char s[3];
	T m_t;
};

class BucketString {
public:
	BucketString()
		: m_filled_size(0) {}

	void reset(size_t size) {
		m_str = std::make_shared<std::string>(size, 0);
		m_filled_size = 0;
	}

	const auto& get() const {
		return m_str;
	}

	size_t size() const {
		return m_str->size();
	}

	size_t filled_size() const {
		return m_filled_size;
	}

	bool is_full() const {
		return m_filled_size >= m_str->size();
	}

	bool fill(const char*& buf, size_t& len) {
		size_t need = size() - filled_size();
		if (need <= 0)
			return false;

		size_t filled = (std::min)(need, len);
		memcpy(&m_str->at(m_filled_size), buf, filled);
		buf += filled;
		len -= filled;
		m_filled_size += filled;
		return need == filled;
	}

private:
	size_t m_filled_size;
	std::shared_ptr<std::string> m_str;
};
