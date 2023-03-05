#pragma once
#include "Common.h"
// ThreadCache��ÿ���̶߳��еģ�����С��256KB�ڴ�ķ��䡣
// ���߳���ҪС�ڵ���256KB���ڴ�ʱ����ֱ�Ӵ�ThreadCache���롣

class ThreadCache {
public:

	//�����ڴ�
	void* Allocate(size_t size);

	//�ͷ��ڴ�
	void Deallocate(void* ptr, size_t size);

	//��central cache��ȡsize��С�ڴ����
	void* FetchFromCentralCache(size_t index, size_t size);

	
private:
	FreeList _freeLists[NFREE_LISTS];
};

// Thread cache local storage
static _declspec(thread) ThreadCache* tls_threadcache = nullptr;