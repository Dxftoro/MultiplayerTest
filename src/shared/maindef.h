#pragma once

#include <ctime>
#include <chrono>

#define NULL_CLIENT		UINT32_MAX

using id_t = uint32_t;
using timePoint = std::chrono::time_point<std::chrono::steady_clock>;

namespace time {
	using microseconds	= std::chrono::microseconds;
	using milliseconds	= std::chrono::milliseconds;
	using seconds		= std::chrono::seconds;
	using minutes		= std::chrono::minutes;
	using hours			= std::chrono::hours;

	template<typename T = microseconds>
	time_t now() {
		return std::chrono::duration_cast<T>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	/* Time conversion from X to microseconds */
	template<typename T> time_t from(time_t t);
	template<typename T> time_t from<milliseconds>(time_t t) { return t * 1000; }
	template<typename T> time_t from<seconds>(time_t t) { return t * 1000000; }
	template<typename T> time_t from<minutes>(time_t t) { return t * 1000000 * 60; }
	template<typename T> time_t from<hours>(time_t t) { return t * 1000000 * 3600; }

	/* Time conversion from microseconds to X */
	template<typename T> time_t to(time_t t);
	template<typename T> time_t to<milliseconds>(time_t t) { return t / 1000; }
	template<typename T> time_t to<seconds>(time_t t) { return t / 1000000; }
	template<typename T> time_t to<minutes>(time_t t) { return t / (1000000 * 60); }
	template<typename T> time_t to<hours>(time_t t) { return t / (1000000 * 3600); }
}