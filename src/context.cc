/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#include "../include/context.h"
#include "../include/parameter.h"
#include <stdlib.h>

using namespace netco;

Context::Context(size_t stackSize)
	: pStack_(nullptr), stackSize_(stackSize)
{
}

Context::~Context()
{
	if (pStack_)
	{
		free(pStack_);
	}
}

void Context::makeContext(void (*func)(), Processor *pP, Context *pLink)
{
	if (nullptr == pStack_)
	{
		pStack_ = malloc(stackSize_);
	}
	::getcontext(&ctx_);
	ctx_.uc_stack.ss_sp = pStack_; // 分配一个新堆栈
	ctx_.uc_stack.ss_size = parameter::coroutineStackSize;
	ctx_.uc_link = pLink->getUCtx(); // 定义一个后继上下文

	/*
		当上下文通过setcontext或者swapcontext激活后，执行func函数，argc为func的参数个数，后面是func的参数序列;
		当func执行返回后，继承的上下文被激活，如果继承上下文为NULL时，线程退出。
	*/
	makecontext(&ctx_, func, 1, pP);
}

void Context::makeCurContext()
{
	::getcontext(&ctx_);
}

void Context::swapToMe(Context *pOldCtx)
{
	if (nullptr == pOldCtx)
	{
		setcontext(&ctx_);
	}
	else
	{
		swapcontext(pOldCtx->getUCtx(), &ctx_);
	}
}