#pragma once
#include <atomic>

namespace asyncio {

template <typename T>
class ObjCounter {
public:
	ObjCounter() {
		++m_count;
		++m_max_count;
	}

	ObjCounter(const ObjCounter<T>&) {
		++m_count;
		++m_max_count;
	}

	~ObjCounter() {
		--m_count;
	}

	static int GetCount() {
		return m_count;
	}

	static int GetMaxCount() {
		return m_max_count;
	}

private:
	static std::atomic_int32_t m_count;
	static std::atomic_int32_t m_max_count;
};

template <typename T>
std::atomic_int32_t ObjCounter<T>::m_count = 0;

template <typename T>
std::atomic_int32_t ObjCounter<T>::m_max_count = 0;

template <typename T>
class Countable : public ObjCounter<T> {
public:
	Countable(const T& t)
		: m_t(t) {}

	T& operator()() {
		return m_t;
	}

private:
	T m_t;
};

} // namespace asyncio
