#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <assert.h>
#include <time.h>
#include <windows.h>
using std::cout;
using std::endl;


//����һ��MAX_BYTES,���������ڴ��С��MAX_BYTES,�ʹ�thread_cache����
//����MAX_BYTES, ��ֱ�Ӵ�page_cache������
static const size_t MAX_BYTES = 256 * 1024;


//thread_cache��central cache�����������ϣͰ�ı��С
static const size_t NFREE_LISTS = 208;

//Page cache�й�ϣͰ�ı��С
static const size_t NPAGES = 129;

//ҳ��Сת��ƫ������һҳΪ2^13B��Ҳ����8KB    ����С����13λ�ɵõ���Ҫ����ҳ
static const size_t PAGE_SHIFT = 13;

//��������
//32λ�����£�_WIN32�ж��壬_WIN64�޶���
//64λ�����£�_WIN32��_WIN64���ж���
#ifdef _WIN64
typedef unsigned long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#endif // _WIN64


//ֱ��ȥ��������kҳ�Ŀռ�  
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage * (1 << PAGE_SHIFT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//��linux������ 
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}

//��ȡ�ڴ�����д洢��ͷ4bit����8bitֵ����������һ������ĵ�ַ������ *&
static inline void*& NextObj(void* obj) {
	return *(void**)obj;
}

//�����зֺõ�С�ڴ�������������
class FreeList {
public:

	void Push(void* obj) {
		//ͷ��
		//*(void**)obj = _freeList;
		assert(obj);
		NextObj(obj) = _freeList;
		_freeList = obj;
		++_size;
	}

	void PushRange(void* start, void* end,size_t n) {
		NextObj(end) = _freeList;
		_freeList = start;
		_size += n;
	}

	void PopRange(void*& start, void*& end, size_t n) {
		assert(n >= _size);
		start = _freeList;
		end = start;
		for (size_t i = 0; i < n - 1; i++) {
			end = NextObj(end);
		}
		_freeList = NextObj(end);
		NextObj(end) = nullptr;
		_size -= n;
	}

	void* Pop() {
		//ͷɾ
		assert(_freeList);
		void* obj = _freeList;
		_freeList = NextObj(obj);
		--_size;

		return obj;
	}

	//�ж�_freeList�Ƿ�Ϊ��
	bool IsEmpty() {
		return _freeList == nullptr;
	}

	size_t& MaxSize() {
		return _maxSize;
	}

	size_t Size() {
		return _size;
	}
private:
	void* _freeList = nullptr;
	size_t _maxSize = 1;
	size_t _size;
};


/********************************************************
 * ��������Ĺ�ϣͰλ�ú������ڴ���С��ӳ���ϵ				 *
 * ThreadCache�ǹ�ϣͰ�ṹ��ÿ��Ͱ��Ӧ��һ����������			 *
 * ÿ�����������У�����Ŀ�Ĵ�С�ǹ̶��ġ�						 *
 * ÿ��Ͱ�ǰ��������������ڴ��Ĵ�С����Ӧӳ��λ�õġ�			 *
 * �����1B��256KBÿ���ֽڶ�����һ����ϣͰ���Ⲣ����ʵ��			 *
 * ��ˣ����Բ��ö������ķ�ʽ����ĳ����Χ���ڴ���Сӳ�䵽ĳ��Ͱ	 *
 ********************************************************/

 //��������С��ӳ�����
class SizeCLass {
public:
	// ������������11%���ҵ�����Ƭ�˷�            ����208��Ͱ
	// [1,128]                  8byte����        [0,16)�Ź�ϣͰ    16��Ͱ
	// [128+1,1024]             16byte����       [16,72)�Ź�ϣͰ	  56��Ͱ
	// [1024+1,8*1024]          128byte����      [72,128)�Ź�ϣͰ  56��Ͱ
	// [8*1024+1,64*1024]       1024byte����     [128,184)�Ź�ϣͰ 56��Ͱ
	// [64*1024+1,256*1024]     8*1024byte����   [184,208)�Ź�ϣͰ 24��Ͱ


	//������Ҫ���ֽ����Ͷ�����������ʵ��������ڴ���С,��ȡ���϶��뷽��
	static inline size_t _RoundUp(size_t bytes, size_t alignNum) {
		return (bytes + alignNum - 1) & ~(alignNum - 1);
	}
	static inline size_t RoundUp(size_t size) {
		if (size <= 128) {
			return _RoundUp(size, 8);
		}
		else if (size <= 1024) {
			return _RoundUp(size, 16);
		}
		else if (size <= 8 * 1024) {
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024) {
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024) {
			return _RoundUp(size, 8 * 1024);
		}
		else {
			assert(false);
			return -1;
		}
	}
	//align_shift�Ƕ�������ƫ���������������8KB��ƫ��������13
	static inline size_t _Index(size_t bytes, size_t align_shift) {
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
	}
	//������������ڴ���С��Ѱ�����ĸ���������Ͱ
	static inline size_t Index(size_t bytes) {
		assert(bytes <= MAX_BYTES);
		//��һ������洢��ÿ�������ж��ٸ�Ͱ
		static int group[4] = { 16, 56, 56, 56 };
		if (bytes <= 128) {
			return _Index(bytes, 3);  //8B����  16��Ͱ
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group[0];  //16B����  56��Ͱ
		}
		else if (bytes <= 8 * 1024) {
			return _Index(bytes - 1024, 7) + group[0] + group[1];  //128B����  56��Ͱ
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes - 8 * 1024, 10) + group[0] + group[1] + group[2];  //1024B����  56��Ͱ
		}
		else if (bytes <= 256 * 1024) {		//8*1024B����  24��Ͱ
			return _Index(bytes - 64 * 1024, 13) + group[0] + group[1] + group[2] + group[3];  //8*1024B����
		}
		else {
			assert(false);
			return -1;
		}
	}

	//thread cacheһ�δ�central cache��ȡ���ٸ�  ��������sizeָ����С�ڴ���С
	static size_t NumMoveSize(size_t size) {
		assert(size > 0);

		//num����2��512֮��
		//�����һ�λ�õ��٣�С����һ�λ�õĶ�,�����ڴ���˷�
		int num = MAX_BYTES / size;
		if (num < 2)
			num = 2;
		if (num > 512)
			num = 512;

		return num;
	}

	//central cacheһ�δ�page cache��ȡ����ҳ  ��������sizeָ����С�ڴ���С
	//�������� 8B...
	// ...
	//�������� 256KB...
	static size_t NumMovePage(size_t size) {
		assert(size > 0);
		size_t num = NumMoveSize(size);
		size_t bytes = num * size;//һ����Ҫ����byte

		int pageNum = bytes >> PAGE_SHIFT;

		if (pageNum == 0) {//����һҳ���һҳ
			pageNum = 1;
		}
		return pageNum;
	}

private:

};

// PageCache��CentralCache��Ͱ�ҵĶ�����ҳΪ��λSpan����,
// Span�������Դ�ͷ˫��ѭ���������ʽ���ڵ�
// ����������ҳ����ڴ��ȵĽṹ
class Span {
public:

	size_t _pageID = 0;//����ڴ���ʼҳ��ҳ��
	size_t _n = 0;//ҳ������

	Span* _next = nullptr;//Span˫������ĵĽṹ
	Span* _prev = nullptr;

	size_t _useCount = 0;//�к�С���ڴ棬�������thread cache�ļ���
	void* _freeList = nullptr;//�кõ�С���ڴ����������
};

//��ͷ˫��ѭ��������ͷ��㣬����O(1)��ɾ
class SpanList {
public:
	SpanList() {//���캯��
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}


	Span* Begin() {
		return _head->_next;
	}
	Span* End() {
		return _head;
	}

	bool IsEmpty() {
		return _head == _head->_next;
	}


	void Insert(Span* pos, Span* newSpan) {
		assert(pos);
		assert(newSpan);
		Span* prev = pos->_prev;
		//prev ->  newSpan ->  pos
		prev->_next = newSpan;
		newSpan->_prev = prev;
		newSpan->_next = pos;
		pos->_prev = newSpan;
	}

	void Erase(Span* pos) { //erase��û��delete��pos,ֻ���������������
		assert(pos);
		assert(pos != _head);//����ɾͷ���

		Span* prev = pos->_prev;
		prev->_next = pos->_next;
		pos->_next->_prev = prev;

	}

	//ͷ��
	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}

	//����ͷ
	Span* PopFront()
	{
		Span* front = _head->_next;
		Erase(front);
		return front;
	}


private:
	Span* _head;
public:
	std::mutex _mtx;//Ͱ��
};
