#include <unordered_map>

// 哈希表实现O(1)的查找，双向链表实现O(1)的节点插入/删除
// head_和tail_为哨兵节点，避免处理空指针
// 待完善（模版支持，线程安全，容量动态调整，超时淘汰）
struct DLinkNode
{
	int key, value;
	DLinkNode* prev;
	DLinkNode* next;

	DLinkNode() : key(0), value(0), prev(nullptr), next(nullptr)
	{
	}

	DLinkNode(int k, int v) : key(k), value(v), prev(nullptr), next(nullptr)
	{
	}
};

class LRUCache
{
public:
	LRUCache(int cap) : capacity_(cap)
	{
		head_ = new DLinkNode();
		tail_ = new DLinkNode();
		head_->next = tail_;
		tail_->prev = head_;
	}

	int Get(int key)
	{
		if (!cache_.count(key))
		{
			return -1;
		}

		DLinkNode* node = cache_[key];
		MoveToHead(node);
		return node->value;
	}

	void Put(int key, int value)
	{
		if (cache_.count(key))
		{
			DLinkNode* node = cache_[key];
			node->value = value;
			MoveToHead(node);
		}
		else
		{
			if (cache_.size() == capacity_)
			{
				// 触发淘汰策略
				RemoveTail();
			}
			DLinkNode* node = new DLinkNode(key, value);
			AddToHead(node);
			cache_[key] = node;
		}
	}

	// 移动节点到头部
	void MoveToHead(DLinkNode* node)
	{
		RemoveNode(node);
		AddToHead(node);
	}

	// 删除尾部节点
	void RemoveTail()
	{
		DLinkNode* last = tail_->prev;
		RemoveNode(last);
		cache_.erase(last->key);
		delete last;
	}

	// 节点删除
	void RemoveNode(DLinkNode* node)
	{
		if (node->prev) node->prev->next = node->next;
		if (node->next) node->next->prev = node->prev;
	}

	// 头部加入新节点
	void AddToHead(DLinkNode* node)
	{
		node->prev = head_;
		node->next = head_->next;
		head_->next->prev = node;
		head_->next = node;
	}

private:
	int capacity_;
	DLinkNode* head_;
	DLinkNode* tail_;
	std::unordered_map<int, DLinkNode*> cache_;
};
