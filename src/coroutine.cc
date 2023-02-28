/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "../include/coroutine.h"
#include "../include/processor.h"
#include "../include/parameter.h"

using namespace cppCo;

static void coWrapFunc(Processor *pP)
{
	pP->getCurRunningCo()->startFunc();
	pP->killCurCo();
}

Coroutine::Coroutine(Processor *pMyProcessor, size_t stackSize, std::function<void()> &&func)
	: coFunc_(std::move(func)), pMyProcessor_(pMyProcessor), status_(CO_DEAD), ctx_(stackSize)
{
	status_ = CO_READY;
}

Coroutine::Coroutine(Processor *pMyProcessor, size_t stackSize, std::function<void()> &func)
	: coFunc_(func), pMyProcessor_(pMyProcessor), status_(CO_DEAD), ctx_(stackSize)
{
	status_ = CO_READY;
}

Coroutine::~Coroutine()
{
}

void Coroutine::resume()
{
	Context *pMainCtx = pMyProcessor_->getMainCtx();
	switch (status_)
	{
	case CO_READY:
		status_ = CO_RUNNING;
		ctx_.makeContext((void (*)(void))coWrapFunc, pMyProcessor_, pMainCtx);
		ctx_.swapToMe(pMainCtx);
		break;

	case CO_WAITING: // 这个状态是之前被挂起的，挂起的时候协程内是保存了当时的上下文信息的，不需要重新makeContext，所以可以直接swap
		status_ = CO_RUNNING;
		ctx_.swapToMe(pMainCtx);
		break;

	default:
		break;
	}
}

void Coroutine::yield()
{
	status_ = CO_WAITING;
};