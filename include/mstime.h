/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#pragma once
#include <stdint.h>
#include <time.h>

struct timespec;

namespace cppCo
{

	// 一年中每个月的天数，非闰年
	const char days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	// 时间类，单位：毫秒ms
	class Time
	{
	public:
		Time(int64_t msSinceEpoch) : timeVal_(msSinceEpoch) {}

		Time(const Time &time) { timeVal_ = time.timeVal_; }

		Time(const Time &&time) { timeVal_ = time.timeVal_; }

		Time &operator=(const Time &time)
		{
			timeVal_ = time.timeVal_;
			return *this;
		}

		~Time() {}

		// 1970年1月1日到现在的毫秒数
		static Time now();

		// 1970年1月1日到现在的秒数
		static time_t nowSec();

		// 根据距离1970-01-01 00:00:00的秒数和与秒数所属时区的时差计算当前时区的时间
		static void toLocalTime(time_t second, long timezone, struct tm *tm_time);

		// 到现在的时间
		struct timespec timeIntervalFromNow();

		int64_t getTimeVal() { return timeVal_; }

	private:
		int64_t timeVal_;

		// 粗糙时间，每次调用now或UpdataRoughTime会更新该值，不需要精确时间的场所可以调用GetRoughTime获取该时间。单位为秒s
		//  static volatile time_t _roughTime;
	};

	inline bool operator<(Time lhs, Time rhs)
	{
		return lhs.getTimeVal() < rhs.getTimeVal();
	}

	inline bool operator<=(Time lhs, Time rhs)
	{
		return lhs.getTimeVal() <= rhs.getTimeVal();
	}

	inline bool operator>(Time lhs, Time rhs)
	{
		return lhs.getTimeVal() > rhs.getTimeVal();
	}

	inline bool operator>=(Time lhs, Time rhs)
	{
		return lhs.getTimeVal() >= rhs.getTimeVal();
	}

	inline bool operator==(Time lhs, Time rhs)
	{
		return lhs.getTimeVal() == rhs.getTimeVal();
	}

}
