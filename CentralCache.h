#pragma once
#include "Common.h"

//ȫ��ֻ��һ��CentralCache����˿���ʹ�õ�������ģʽ
class CentralCache {
public:

	static CentralCache* GetInstance() {
		return &_sInst;
	}

	//�����Ļ����ȡһ�������Ķ����thread cache 
	size_t FetchRangObj(void*& start, void*& end, size_t batchNum, size_t size);

private:
	SpanList _spanLists[NFREE_LISTS];
private:
	CentralCache(){}

	CentralCache(const CentralCache&) = delete;

	static CentralCache _sInst;
};