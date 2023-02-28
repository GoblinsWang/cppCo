/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "../include/co_api.h"

void cppCo::co_go(std::function<void()> &&func, size_t stackSize, int tid)
{
	if (tid < 0)
	{
		cppCo::Scheduler::getScheduler()->createNewCo(std::move(func), stackSize);
	}
	else
	{
		tid %= cppCo::Scheduler::getScheduler()->getProCnt();
		cppCo::Scheduler::getScheduler()->getProcessor(tid)->goNewCo(std::move(func), stackSize);
	}
}

void cppCo::co_go(std::function<void()> &func, size_t stackSize, int tid)
{
	if (tid < 0)
	{
		cppCo::Scheduler::getScheduler()->createNewCo(func, stackSize);
	}
	else
	{
		tid %= cppCo::Scheduler::getScheduler()->getProCnt();
		cppCo::Scheduler::getScheduler()->getProcessor(tid)->goNewCo(func, stackSize);
	}
}

void cppCo::co_sleep(Time time)
{
	cppCo::Scheduler::getScheduler()->getProcessor(threadIdx)->wait(time);
}

void cppCo::sche_join()
{
	cppCo::Scheduler::getScheduler()->join();
}