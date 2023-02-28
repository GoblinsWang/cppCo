/***
	@author: Wangzhiming
	@date: 2022-10-29
***/
#pragma once
#include "parameter.h"
#include "utils.h"

namespace netco
{
	struct MemBlockNode
	{
		union
		{
			MemBlockNode *next;
			char data;
		};
	};

	// 每次可以从内存池中获取objSize大小的内存块
	template <size_t objSize>
	class MemPool
	{
	public:
		MemPool()
			: _freeListHead(nullptr), _mallocListHead(nullptr), _mallocTimes(0)
		{
			if (objSize < sizeof(MemBlockNode))
			{
				objSize_ = sizeof(MemBlockNode);
			}
			else
			{
				objSize_ = objSize;
			}
		};

		~MemPool();

		DISALLOW_COPY_MOVE_AND_ASSIGN(MemPool);

		void *AllocAMemBlock();
		void FreeAMemBlock(void *block);

	private:
		// 空闲链表
		MemBlockNode *_freeListHead;
		// malloc的大内存块链表
		MemBlockNode *_mallocListHead;
		// 实际malloc的次数
		size_t _mallocTimes;
		// 每个内存块大小
		size_t objSize_;
	};

	template <size_t objSize>
	MemPool<objSize>::~MemPool()
	{
		while (_mallocListHead)
		{
			MemBlockNode *mallocNode = _mallocListHead;
			_mallocListHead = mallocNode->next;
			free(static_cast<void *>(mallocNode));
		}
	}

	template <size_t objSize>
	void *MemPool<objSize>::AllocAMemBlock()
	{
		void *ret;
		if (nullptr == _freeListHead)
		{
			size_t mallocCnt = parameter::memPoolMallocObjCnt + _mallocTimes;
			void *newMallocBlk = malloc(mallocCnt * objSize_ + sizeof(MemBlockNode));
			MemBlockNode *mallocNode = static_cast<MemBlockNode *>(newMallocBlk);
			mallocNode->next = _mallocListHead;
			_mallocListHead = mallocNode;
			newMallocBlk = static_cast<char *>(newMallocBlk) + sizeof(MemBlockNode);
			for (size_t i = 0; i < mallocCnt; ++i)
			{
				MemBlockNode *newNode = static_cast<MemBlockNode *>(newMallocBlk);
				newNode->next = _freeListHead;
				_freeListHead = newNode;
				newMallocBlk = static_cast<char *>(newMallocBlk) + objSize_;
			}
			++_mallocTimes;
		}
		ret = &(_freeListHead->data);
		_freeListHead = _freeListHead->next;
		return ret;
	}

	template <size_t objSize>
	void MemPool<objSize>::FreeAMemBlock(void *block)
	{
		if (nullptr == block)
		{
			return;
		}
		MemBlockNode *newNode = static_cast<MemBlockNode *>(block);
		newNode->next = _freeListHead;
		_freeListHead = newNode;
	}
}
