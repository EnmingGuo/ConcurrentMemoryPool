#pragma once
#include "Common.h"
#include "ThreadCache.h"



//�����ڴ�  //ͨ��TLSÿ���߳������ش���thread_cache
static void* ConcurrentAlloc(size_t size) {
	if (tls_threadcache == nullptr) {
		tls_threadcache = new ThreadCache;
	}
	//cout << std::this_thread::get_id() << " : " << tls_threadcache << endl;

	return tls_threadcache->Allocate(size);
}

//�ͷ��ڴ�
static void ConcurrentFree(void* ptr, size_t size) {
	assert(tls_threadcache);
	tls_threadcache->Deallocate(ptr, size);
}