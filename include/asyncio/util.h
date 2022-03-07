/* asyncio implementation.
 *
 * Copyright (c) 2019-2022, Will <will at i007 dot cc>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of asyncio nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	#include <stdlib.h>
	#include <windows.h>
	#include <io.h>
#elif __APPLE__
	#include <dirent.h>
	#include <unistd.h>
#else
	#include <unistd.h>
	#include <sys/io.h>
	#include <dirent.h>
	#include <sys/stat.h>
#endif

namespace asyncio {
namespace util {

template <class T>
inline void UNUSED(T const&) {}

class Random {
public:
	template <class T>
	static T RandomInt(T low, T high) {
		static std::random_device rd;
		static std::default_random_engine engine(rd());

		std::uniform_int_distribution<T> dis(0, high - low);
		T dice_roll = dis(engine) + low;
		return dice_roll;
	}
};

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

class Text {
public:
	// 不区分大小的字符串比较
	static int StrCaseCmp(const char* s1, const char* s2) {
#ifdef _WIN32
		return _stricmp(s1, s2);
#else
		return strcasecmp(s1, s2);
#endif
	}

	static std::string& Format(std::string& s, const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);

		va_list args_copy;
		va_copy(args_copy, args);
		size_t len = std::vsnprintf(nullptr, 0, fmt, args);
		if (len > 0) {
			s.resize(len + 1);
			std::vsnprintf(&s[0], s.size(), fmt, args_copy);
		}

		va_end(args);
		return s;
	}

	static std::string Format(const char* fmt, ...) {
		std::string s;
		va_list args;
		va_start(args, fmt);

		va_list args_copy;
		va_copy(args_copy, args);
		size_t len = std::vsnprintf(nullptr, 0, fmt, args);
		if (len > 0) {
			s.resize(len + 1);
			std::vsnprintf(&s[0], s.size(), fmt, args_copy);
		}

		va_end(args);
		return s.substr(0, len);
	}

	// 主要用来切分使用空格分隔的字符串，连续的空格算作一个分隔符
	static size_t SplitStr(std::vector<std::string>& os, const std::string& is, char c) {
		os.clear();
		auto start = is.find_first_not_of(c, 0);
		while (start != std::string::npos) {
			auto end = is.find_first_of(c, start);
			if (end == std::string::npos) {
				os.emplace_back(is.substr(start));
				break;
			} else {
				os.emplace_back(is.substr(start, end - start));
				start = is.find_first_not_of(c, end + 1);
			}
		}
		return os.size();
	}

	static size_t SplitInt(std::vector<int>& number_result, const std::string& is, char c) {
		std::vector<std::string> string_result;
		SplitStr(string_result, is, c);

		number_result.clear();
		for (size_t i = 0; i < string_result.size(); i++) {
			const std::string& value = string_result[i];
			number_result.emplace_back(atoi(value.data()));
		}

		return number_result.size();
	}

	// 拆分用特殊字符分隔的布尔字符串
	static size_t SplitBool(std::vector<bool>& vec, const std::string& is, const char c) {
		std::vector<std::string> vec_string;
		SplitStr(vec_string, is, c);

		vec.clear();
		for (size_t i = 0; i < vec_string.size(); i++) {
			const std::string& value = vec_string[i];
			vec.push_back(atoi(value.data()) != 0);
		}

		return vec.size();
	};

	static std::vector<std::string> ParseParam(const std::string& is, char c) {
		std::vector<std::string> result;
		ParseParam(result, is, c);
		return result;
	}

	// 主要用来切分使用逗号分隔的字符串，连续的逗号算作多个分隔符
	static size_t ParseParam(std::vector<std::string>& result, const std::string& is, char c) {
		result.clear();
		size_t start = 0;
		while (start < is.size()) {
			auto end = is.find_first_of(c, start);
			if (end != std::string::npos) {
				result.emplace_back(is.substr(start, end - start));
				start = end + 1;
			} else {
				result.emplace_back(is.substr(start));
				break;
			}
		}

		if (start == is.size()) {
			result.emplace_back(std::string());
		}
		return result.size();
	}
};

class App {
public:
	//完整路径,程序名称（不带.exe后缀）
	// Windows下形如：D:/git/gms/bin/app/Debug, BalanceServer
	static std::pair<std::string, std::string> GetAppName() {
		std::string fullName;
		static const int MAXBUFSIZE = 1024;
		char path[MAXBUFSIZE] = {0};
#ifdef _WIN32
		LPTSTR szFullPath = (LPTSTR)&path;
		if (::GetModuleFileName(NULL, szFullPath, MAXBUFSIZE)) {
			fullName.assign(path);
		}
		std::replace(fullName.begin(), fullName.end(), '\\', '/');
#else
		int count = readlink("/proc/self/exe", path, MAXBUFSIZE);
		if (count > 0 && count < MAXBUFSIZE) {
			path[count] = '\0';
			fullName.assign(path);
		} else {
			char cmdline[MAXBUFSIZE] = {0};
			FILE* fp = NULL;

			sprintf(cmdline, "ls -lt /proc/%d | grep exe | awk '{print $NF}'", getpid());
			if ((fp = popen(cmdline, "r"))) {
				if (fgets(path, MAXBUFSIZE, fp)) {
					size_t len = strlen(path);
					if (len > 0) {
						if ('\n' == path[--len])
							path[len] = '\0';
						fullName.assign(path);
					}
				}
				pclose(fp);
			}
		}
#endif

		auto it = fullName.find_last_of("/");
		auto fullPath = fullName.substr(0, it);
		auto appName = fullName.substr(it + 1);
		appName = appName.substr(0, appName.find_first_of("."));

		return std::make_pair(fullPath, appName);
	}

	static std::string GetParentDir(const std::string& dir) {
		return dir.substr(0, dir.find_last_of('/'));
	}

	//获取一个目录下面所有的文件
	static std::vector<std::string> GetAllFilesFromDir(const std::string& dir) {
		std::vector<std::string> ret;
#ifdef _WIN32
		std::string dir_filter = dir + "/*.*";
		_finddata_t findData;
		intptr_t handle = _findfirst(dir_filter.data(), &findData);
		if (handle == -1) {
			return ret;
		}

		do {
			if (findData.attrib & _A_SUBDIR) {
				// 是否是子目录并且不为"."或".."
			} else {
				ret.push_back(findData.name);
			}
		} while (_findnext(handle, &findData) == 0);

		_findclose(handle);
#else
		struct dirent* ptr;
		DIR* _dir = opendir(dir.data());
		while ((ptr = readdir(_dir)) != NULL) {
			if (ptr->d_type == DT_DIR)
				continue;
			ret.push_back(ptr->d_name);
		}
		closedir(_dir);
#endif // _WIN32
		return ret;
	}
};

} // namespace util
} // namespace asyncio
