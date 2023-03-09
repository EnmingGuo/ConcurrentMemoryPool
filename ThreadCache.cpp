#include "ThreadCache.h"
#include "CentralCache.h"

//windows.h����min max�궨�壬�Ὣalgorithm�е�min max����ģ�浱������������Ҫ�ڴ˽����
#undef max
#undef min

void* ThreadCache::Allocate(size_t size) {

	assert(size <= MAX_BYTES);
	size_t allocSize = SizeCLass::RoundUp(size);//��������ʵ�ʷ�����ڴ��С  allocSize>=size
	size_t index = SizeCLass::Index(size);//����ӳ�䵽�ĸ�Ͱ


	//�����Ӧ����������Ͱ��Ϊ�գ�ֱ�Ӵ�Ͱ��ȡ���ڴ��
	//���Ϊ�գ����central_cache�л�ȡ�ڴ��
	if (!_freeLists[index].IsEmpty()) {
		return _freeLists[index].Pop();
	}
	else {
		return FetchFromCentralCache(index, allocSize);
	}
}
void ThreadCache::Deallocate(void* ptr, size_t size) {
	assert(size <= MAX_BYTES);
	assert(ptr);
	size_t index = SizeCLass::Index(size);//����ӳ�䵽�ĸ�Ͱ
	_freeLists[index].Push(ptr);//ͷ��

}

//��������ڴ����256KBʱ����central cache����,Ȼ��������ڴ����뵽_freeLists[index]��
//���ҷ���ͷһ���ڴ�顣
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size) {
	//��ȡ����ʼ������������
	// 1. �ʼ������central cacheҪ̫�࣬��Ϊ̫���˿����ò����˷�
	// 2. ��������������batchNum����������ֱ������NumMoveSize;
	// 3. sizeԽ��һ����central cacheҪ��batchNumԽ��
	// 5. sizeԽС��һ����central cacheҪ��batchNumԽС
	size_t batchNum = std::min( _freeLists[index].MaxSize(), SizeCLass::NumMoveSize(size) );

	if (_freeLists[index].MaxSize() == batchNum) {
		_freeLists[index].MaxSize() += 1;
	}

	void* start = nullptr;
	void* end = nullptr;

	size_t actualNum = CentralCache::GetInstance()->FetchRangObj(start, end, batchNum, size);
	assert(actualNum > 1);//����Ҫ��thread cacheһ��
	if (actualNum == 1) {
		assert(start == end);
	}
	else {
		//[start,end]��thread cache��central cache������ڴ�鷶Χ
		//������Ҫ��[start,end]���뵽_freeLists[index]�У����ҷ��ص�һ���ڴ��
		//�������д���ȼ���
			/*	_freeLists[index].PushRange(start, end);
				return _freeLists[index].Pop();			*/

		_freeLists[index].PushRange(NextObj(start), end);
	}
	return start;
}
