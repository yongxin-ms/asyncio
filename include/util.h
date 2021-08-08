#pragma once
#include <string>
#include <vector>
#include <random>
#include <ctime>
#include <chrono>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#ifdef _WIN32
#include <stdlib.h>
#include <windows.h>
#include <io.h>
#elif __APPLE__
#include <dirent.h>
#else
#include <unistd.h>
#include <sys/io.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace asyncio {
namespace util {
class Time {
public:
	static tm LocalTime(time_t t) {
		tm this_tm;
#ifdef _WIN32
		localtime_s(&this_tm, &t);
#else
		localtime_r(&t, &this_tm);
#endif // _WIN32
		return this_tm;
	}

	//把时间转换成字符串表示
	static std::string FormatDateTime(time_t t) {
		tm this_tm = LocalTime(t);
		char data[64] = {0};
		std::snprintf(data, sizeof(data) - 1, "%d-%02d-%02d %02d:%02d:%02d", this_tm.tm_year + 1900, this_tm.tm_mon + 1,
					  this_tm.tm_mday, this_tm.tm_hour, this_tm.tm_min, this_tm.tm_sec);
		return std::string(data);
	}

	//带毫秒的时间字符串
	static std::string FormatDateTime(const std::chrono::system_clock::time_point& t) {
		uint64_t mill = std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count() -
						std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count() * 1000;
		char data[64] = {0};
		tm this_tm = LocalTime(std::chrono::system_clock::to_time_t(t));
		std::snprintf(data, sizeof(data) - 1, "%d-%02d-%02d %02d:%02d:%02d.%03" PRIu64 "", this_tm.tm_year + 1900,
					  this_tm.tm_mon + 1, this_tm.tm_mday, this_tm.tm_hour, this_tm.tm_min, this_tm.tm_sec, mill);
		return std::string(data);
	}

	static time_t StrToDateTime(const char* input) {
		if (strlen(input) != strlen("1970-01-01 08:00:00"))
			return 0;
		tm this_tm = LocalTime(time(nullptr));
#ifdef _WIN32
		sscanf_s(input,
#else
		sscanf(input,
#endif
				 "%d-%02d-%02d %02d:%02d:%02d", &this_tm.tm_year, &this_tm.tm_mon, &this_tm.tm_mday, &this_tm.tm_hour,
				 &this_tm.tm_min, &this_tm.tm_sec);
		this_tm.tm_year -= 1900;
		this_tm.tm_mon -= 1;
		return mktime(&this_tm);
	}
};

}
}
