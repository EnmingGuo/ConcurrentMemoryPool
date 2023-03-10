#include "CentralCache.h"
#include "PageCache.h"

CentralCache CentralCache::_sInst;

//�����Ļ����ȡһ���ǿյ�Span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size) {
	//�Ȳ鿴��ǰ��spanlist���Ƿ���δ��������span
	Span* it = list.Begin();
	while (it != list.End()) {
		if (it->_freeList) {//ֻҪ��ǰSpan�µ�_freeList��Ϊ��
			return it;
		}
		else {
			it = it->_next;
		}
	}


	//�Ȱ�central cache��Ͱ�����ˣ�����������������ͷ��ڴ������������
	list._mtx.unlock();

	//�ߵ�����˵��û�п���span�ˣ�ֻ����page cacheҪ�ڴ�,Ҫһ��span
	PageCache::GetInstance()->_pageMtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeCLass::NumMovePage(size));
	PageCache::GetInstance()->_pageMtx.unlock();
	//��NewSpan()������������page cache�����󣬲���Ҫ��central cache��Ͱ��
	//��Ϊ�����߳��ò������span
	

	//span�Ĵ���ڴ���ʼ��ַ��������ַ
	char* startAddress = (char*)(span->_pageID << PAGE_SHIFT);
	
	//span�Ĵ���ڴ��ֽ���
	size_t bytes = span->_n << PAGE_SHIFT;
	char* endAddress = startAddress + bytes;

	//�Ѵ���ڴ��г�����������������
	//1.����һ��������ͷ������β��
	span->_freeList = startAddress;
	startAddress += size;
	//2.β��
	void* tail = span->_freeList;
	
	while (startAddress < endAddress) {
		NextObj(tail) = startAddress;
		tail = NextObj(tail);
		startAddress += size;
	}

	//�к�span�Ժ���Ҫ��span�ҵ�Ͱ����ȥ��ʱ���ټ���
	list._mtx.lock();
	list.PushFront(span);
	return span;
}


//�����Ļ����ȡһ�������Ķ����thread cache
// central cache�� thread cacheӳ���ϵһ�¡�
//batchNumΪ����ĸ�����sizeΪ�����ڴ��С
//��central cache��span����batchNum���ڴ�飬��ָ�batchNum����û����ô���� ���� ��һ����
size_t CentralCache::FetchRangObj(void*& start, void*& end, size_t batchNum, size_t size) {
	size_t index = SizeCLass::Index(size);
	_spanLists[index]._mtx.lock();//����Ͱ��

	Span* span = GetOneSpan(_spanLists[index], size);
	assert(span);
	assert(span->_freeList);

	start = span->_freeList;
	end = start;

	//��span�л�ȡbatchNum������
	//������batchNum�����ж����ö���

	size_t i = 0;
	size_t actualNum = 1;//ע��actualNum��ʼֵΪ1������0
	while (i < batchNum - 1 && NextObj(end) != nullptr) {
		end = NextObj(end);
		i++;
		actualNum++;
	}

	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;
	span->_useCount += actualNum;

	_spanLists[index]._mtx.unlock();//��Ͱ��

	return actualNum;
}


//��һ���������ڴ���ͷŵ�Span��
void CentralCache::ReleaseListToSpans(void* start, size_t size) {

}