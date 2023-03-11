#pragma once
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"
#include "ObjectPool.h"

//�����ڴ�  //ͨ��TLSÿ���߳������ش���thread_cache

/*
	1.��������ڴ�<=256kB   �������㻺��
	2.��������ڴ� >256kB	   ��
			a.  256kB < size <= NPages(128)*8KB   ->page cache
			b.	size > NPages(128)*8KB    ->��ϵͳ�Ķ�����
*/

static void* ConcurrentAlloc(size_t size) {
	
	if (size > MAX_BYTES) {//����256KB������

		size_t alignSize = SizeCLass::RoundUp(size);
		size_t kPages = alignSize >> PAGE_SHIFT;

		PageCache::GetInstance()->_pageMtx.lock();
		Span* span = PageCache::GetInstance()->NewSpan(kPages);
		span->_objectSize = size;
		PageCache::GetInstance()->_pageMtx.unlock();

		void* ptr = (void*)(span->_pageID << PAGE_SHIFT);
		return ptr;

	}
	else {
		if (tls_threadcache == nullptr) {
			static ObjectPool<ThreadCache>tcPool;
			//tls_threadcache = new ThreadCache;
			tls_threadcache = tcPool.New();
		}
		//cout << std::this_thread::get_id() << " : " << tls_threadcache << endl;

		return tls_threadcache->Allocate(size);
	}
	
}

//�ͷ��ڴ�
static void ConcurrentFree(void* ptr) {
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);//��ȡptr���ڵ�span
	size_t size = span->_objectSize;

	if (size > MAX_BYTES) {//����256KB���ͷ�
		PageCache::GetInstance()->_pageMtx.lock();
		PageCache::GetInstance()->ReleaseSpanToPageCache(span);
		PageCache::GetInstance()->_pageMtx.unlock();
	}
	else {
		assert(tls_threadcache);
		tls_threadcache->Deallocate(ptr, size);
	}

}