#pragma once

#include <chrono>
#define NULL_CLIENT		UINT32_MAX

using id_t = uint32_t;
using timePoint = std::chrono::time_point<std::chrono::steady_clock>;

namespace utime {
	using microseconds	= std::chrono::microseconds;
	using milliseconds	= std::chrono::milliseconds;
	using seconds		= std::chrono::seconds;
	using minutes		= std::chrono::minutes;
	using hours			= std::chrono::hours;

	template<typename T = microseconds>
	std::time_t now() {
		return std::chrono::duration_cast<T>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	/* Time conversion from X to microseconds */
	template<typename T> std::time_t from(std::time_t t);
	template<> std::time_t from<milliseconds>(std::time_t t) { return t * 1000; }
	template<> std::time_t from<seconds>(std::time_t t) { return t * 1000000; }
	template<> std::time_t from<minutes>(std::time_t t) { return t * 1000000 * 60; }
	template<> std::time_t from<hours>(std::time_t t) { return t * 1000000 * 3600; }

	/* Time conversion from microseconds to X */
	template<typename T> std::time_t to(std::time_t t);
	template<> std::time_t to<milliseconds>(std::time_t t) { return t / 1000; }
	template<> std::time_t to<seconds>(std::time_t t) { return t / 1000000; }
	template<> std::time_t to<minutes>(std::time_t t) { return t / (1000000 * 60); }
	template<> std::time_t to<hours>(std::time_t t) { return t / (1000000 * 3600); }
}