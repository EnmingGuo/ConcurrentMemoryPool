#pragma once
#include "Common.h"

//�����ڴ��
template <class T>
class ObjectPool
{
public:
	T* New() {
		T* obj = nullptr;
		//���ȷ��������������滹�������ڴ��
		if (_freeList) {//ͷɾ
			void* next = *((void**)_freeList);
			obj = (T*)_freeList;
			_freeList = next;
		}
		else {
			//���ʣ���ڴ�С��һ�����������¿��ٴ���ڴ�
			if (_remainBytes < sizeof(T)) {
				_remainBytes = 128 * 1024;
				_memory = (char*)SystemAlloc(_remainBytes >> PAGE_SHIFT);//128KB
				if (_memory == nullptr) {//����ʧ�����׳��쳣
					throw std::bad_alloc();
				}
			}
			obj = (T*)_memory;
			//����32λ���£�ָ��Ϊ4B����T����<4B�����޷�����ָ�룬�����޷�������ȥ
			//����64λ���£�ָ��Ϊ8B����T����<8B�����޷�����ָ�룬�����޷�������ȥ
			//�����Ҫ������ж�
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
			_memory += objSize;
			_remainBytes -= objSize;
		}
		//��λnew����Ȼ�Ѿ�Ϊobj��������ڴ棬��ô�Ϳ��Ե���T�Ĺ��캯��������ʽ��ʼ��
		new(obj) T;
		return obj;
	}

	void Delete(T* obj) {
		//��ʽ����T����������
		obj -> ~T();
		//ͷ��
		*((void**)obj) = _freeList;//�ö���ָ�����������32��64λ��
		_freeList = obj;
	}

private:

	char* _memory = nullptr;//ָ���ڴ��
	size_t _remainBytes = 0;//�ڴ�����зֹ�����ʣ���ֽ���
	void* _freeList = nullptr;//�����ڴ�����е����������ͷָ��

};
