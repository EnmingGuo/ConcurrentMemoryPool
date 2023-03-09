#pragma once
#include "Common.h"

//ȫ��ֻ��һ��PageCache����˿���ʹ�õ�������ģʽ
class PageCache {
public:
	static PageCache* GetInstance() {
		return &_sInst;
	}

	//��CentralCache kҳ��С��Span
	Span* NewSpan(size_t k);
public:
	std::mutex _pageMtx;//page cache��������Ͱ�� ����ȫ��������Ϊǣ����ҳ�ķ���(�и�)�ͺϲ����⡣
private:

	SpanList _spanLists[NPAGES];

	PageCache(){}
	PageCache(const PageCache&) = delete;

	static PageCache _sInst;
};