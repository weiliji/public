#pragma  once
#include "Base/IntTypes.h"
#include "Base/Shared_ptr.h"
#include <atomic>

namespace Public{
namespace Base {


//无锁列表
template<typename DATA>
class LockFreeList
{
public:
	LockFreeList() :m_pHead(NULL),m_pTail(NULL),m_Count(0)
	{
		m_pHead = new node();
	}
	~LockFreeList() 
	{
		clear();

		node* header = m_pHead.load();
		if (header) SAFE_DELETE(header);
		m_pHead = NULL;
		m_pTail = NULL;
		m_Count = 0;
	}

	void clear()
	{
		while (size() > 0) pop_front();
	}

	//传进来的已经分配好的点
	void push_back(const DATA& data)
	{
		node* new_node = new node(data);
		node* next = NULL;
		node*  tail = m_pTail.load();

		/*
		*一开始没有数据时,m_pTail为空,这时只需要将m_pTail指指向本节点即可
		*首先抢到该原子操作的线程会更新m_pTail,设置头指针的NEXT(避免push与pop在同一个元素上的互斥),
		*然后返回,而后进入该原子操作的
		*线程只会单纯的更新tail后退出if结构.继续往下执行
		*/
		if (!m_pTail.load())
		{
			if (m_pTail.compare_exchange_weak(tail, new_node))
			{
				m_pHead.load()->next = new_node;
				m_Count.fetch_add(1);
				return;
			}
		}

		/*
		*更新next指针,当tail指针的next为空时,则表示当前tail为尾元素
		*/
		do {
			tail = m_pTail.load();
			next = NULL;
		} while (!tail->next.compare_exchange_weak(next, new_node));

		/*
		* 该m_pTail指针未更新时,do while的循环是持续进行的.因为tail->next != NULL
		1.如果有一个线程T1，它的while中的CAS如果成功的话，那么其它所有的 随后线程的CAS都会失败，然后就会再循环，
		2.此时，如果T1 线程还没有更新tail指针，其它的线程继续失败，因为tail->next不是NULL了。
		3.直到T1线程更新完tail指针，于是其它的线程中的某个线程就可以得到新的tail指针，继续往下走了。
		*/
		m_pTail.compare_exchange_weak(tail, new_node);

		m_Count.fetch_add(1);
	}

	DATA& get(size_t offset)
	{
		node* dataptr = m_pHead.load();
		
		for(size_t i = 0;i < offset;i ++)
		{
			if (dataptr == NULL) return emtpyval;

			dataptr = dataptr->next.load();
		}

		if (dataptr == NULL) return emtpyval;

		node* nextptr = dataptr->next.load();
		if(nextptr == NULL) return emtpyval;

		return nextptr->data;
	}
	DATA& front()
	{
		return get(0);
	}

	void pop_front()
	{
		/*
		*如果队列为空则直接返回
		*/
		if (!m_Count.load()) return;
		node* head = m_pHead.load();
		node* popData;

		do {
			/**每次循环需要重新获取head->next最新值*/
			popData = head->next;

			/*当popData == NULL时,
			*说明元素被其它线程将元素取完了,
			*本线程可以直接返回NULL,
			*而m_pTail会在取到最后一个元素的线程中置NULL*/
			if (!popData) return;

		} while (!head->next.compare_exchange_weak(popData, popData->next));

		/*经过循环的线程,代表着弹出了实际元素的线程,元素减一*/
		m_Count.fetch_sub(1);

		/**弹出最后一个元素的线程需把m_pTail置为NULL*/
		if (!head->next.load()) m_pTail.store(NULL);

		/**这里有ABA的问题.但采用内存池可以避免*/
		SAFE_DELETE(popData);
	}

	bool empty()
	{
		return m_Count.load() == 0;
	}

	size_t size()
	{
		return m_Count.load();
	}

private:
	DATA emtpyval;

	struct node
	{
		node() :next(NULL) {};
		node(const DATA& _data) :data(_data), next(NULL) {}

		DATA data;
		atomic<node*> next;
	};

	atomic<node*> m_pHead;
	atomic<node*> m_pTail;
	atomic<size_t> m_Count;
};


}
}

