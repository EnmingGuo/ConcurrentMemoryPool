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
				_memory = (char*)malloc(128 * 1024);//128KB
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

struct TreeNode {
	int _val;
	TreeNode* _left;
	TreeNode* _right;

	TreeNode()
		:_val(0)
		, _left(nullptr)
		, _right(nullptr)
	{}
};

void TestObjectPool() {

	//���Ե��ִ�
	const size_t Rounds = 3;
	//ÿ�������ͷŴ���
	const size_t N = 100000;

	//����new deleteЧ��
	std::vector<TreeNode*>v1;
	v1.reserve(N);
	size_t begin1 = clock();
	for (int i = 0; i < Rounds; i++) {
		for (int j = 0; j < N; j++) {
			v1.push_back(new TreeNode);
		}
		for (int j = 0; j < N; j++) {
			delete v1[j];
		}
		v1.clear();
	}
	size_t end1 = clock();

	//���Զ����ڴ��Ч��
	std::vector<TreeNode*>v2;
	v2.reserve(N);
	ObjectPool<TreeNode>TNPool;
	size_t begin2 = clock();
	for (int i = 0; i < Rounds; i++) {
		for (int j = 0; j < N; j++) {
			v2.push_back(TNPool.New());
		}
		for (int j = 0; j < N; j++) {
			TNPool.Delete(v2[j]);
		}
		v2.clear();
	}
	size_t end2 = clock();

	cout << "new cost time = " << end1 - begin1 << endl;
	cout << "object pool cost time = " << end2 - begin2 << endl;
}