#pragma once
#include <mutex>

template <typename T>
class Singleton {
public:
	static T* Instance() {
		static std::once_flag oc;
		std::call_once(oc, [&] { _instance = new T; });
		return _instance;
	}

	static void Release() {
		if (_instance != nullptr) {
			delete _instance;
			_instance = nullptr;
		}
	}

protected:
	Singleton() = default;
	~Singleton() = default;

	static T* _instance;

private:
	Singleton(const Singleton&) = delete;
	const Singleton& operator=(const Singleton&) = delete;
};

template <typename T>
T* Singleton<T>::_instance = nullptr;
