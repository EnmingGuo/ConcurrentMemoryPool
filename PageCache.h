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

	//��ȡ�ڴ���Span��ӳ��
	Span* MapObjectToSpan(void* obj);

	//��central cache��span�黹��page cache
	void ReleaseSpanToPageCache(Span* span);
public:
	std::mutex _pageMtx;//page cache��������Ͱ�� ����ȫ��������Ϊǣ����ҳ�ķ���(�и�)�ͺϲ����⡣
private:

	SpanList _spanLists[NPAGES];
	//ҳ����span��ӳ��
	std::unordered_map<PAGE_ID, Span*>_idSpanMap;
	PageCache() {}
	PageCache(const PageCache&) = delete;

	static PageCache _sInst;
};