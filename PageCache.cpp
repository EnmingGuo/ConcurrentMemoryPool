#include "PageCache.h"

PageCache PageCache::_sInst;

//��CentralCache kҳ��С��Span
Span* PageCache::NewSpan(size_t k) {
	assert(k > 0 && k < NPAGES);

	//�ȼ��page cache��k��Ͱ��û��span
	if (!_spanLists[k].IsEmpty()) {
		return _spanLists->PopFront();
	}

	//�ټ��һ�º����Ͱ��û��span������У�����Խ����з�
	for (size_t i = k + 1; k < NPAGES; i++) {
		if (!_spanLists[i].IsEmpty()) {
			//��ʼ�з�,��iҳ��span�зֳ�kҳ��span��һ��i-kҳ��span
			//kҳ��span���ظ�central cache
			//i-kҳ��span�ҵ���i-k��Ͱ��ȥ
			Span* iSpan = _spanLists[i].PopFront();
			Span* kSpan = new Span;

			//��iSpan��ͷ����һ��kҳ����
			kSpan->_pageID = iSpan->_pageID;
			kSpan->_n = k;

			iSpan->_pageID += k;
			iSpan->_n -= k;


			//��i-kҳ��span�ҵ���i-k��Ͱ��ȥ
			_spanLists[iSpan->_n].PushFront(iSpan);

			//����kҳ��span
			return kSpan;
		}
	}

	//�ߵ���һ����˵�������Ҳ�����ҳ��spanȥ�з���
	//��ʱ��ȥ�Ҷ�Ҫһ��128ҳ��span
	Span* bigSpan = new Span;
	void* ptr = SystemAlloc(NPAGES - 1);
	bigSpan->_pageID = (PAGE_ID)ptr >> 13;
	bigSpan->_n = NPAGES - 1;

	_spanLists[bigSpan->_n].PushFront(bigSpan);
	return NewSpan(k); 
}