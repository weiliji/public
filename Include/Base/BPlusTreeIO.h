#pragma once
#include "Base/IntTypes.h"
#include "Base/Shared_ptr.h"
#include "Base/ErrorInfo.h"
#include "Base/Guard.h"
#include "Base/PrintLog.h"

namespace Public
{
namespace Base
{

struct EmptyHeader
{
};

#define MAX_BLOCK_SIZE 4096

//B树的IO操作
struct BASE_API BPlusTreeIOOperation
{
	enum IOType
	{
		IOType_BOOT, //boot数据
		IOType_TREE, //tree数据
	};

	BPlusTreeIOOperation() {}
	virtual ~BPlusTreeIOOperation() {}

	virtual ErrorInfo read(IOType type, off_t offset, void *data, uint32_t count) = 0;
	virtual ErrorInfo write(IOType type, off_t offset, void *data, uint32_t count) = 0;
};

//[Key],为树的key节点，该对象必须重载<和==操作符
//[Data]，为树对应的数据节点
//[MAXBLOCKSIZE],为树单节点的存储位置,sizeof(KEY)+sizeof(DATA) < MAXBLOCKSIZE
//该类是线性安全的
template <typename Key, typename Data = EmptyHeader, uint32_t MAXBLOCKSIZE = MAX_BLOCK_SIZE>
class BPlusTreeIO
{
private:
#define MIN_CACHE_NUM 20
#define MAX_LEVEL 10

#define INVALID_OFFSET -1

#define _offset_ptr(node) ((char *)node + sizeof(*node))
#define _key_ptr(node) ((Key *)_offset_ptr(node))
#define _data_ptr(node) ((Data *)(_offset_ptr(node) + node->maxEntries * sizeof(Key)))
#define _sub_ptr(node) ((off_t *)(_offset_ptr(node) + (node->maxOrder - 1) * sizeof(Key)))
public:
	BPlusTreeIO() {}
	~BPlusTreeIO() { close(); }

	static ErrorInfo check(const shared_ptr<BPlusTreeIOOperation> &_io)
	{
		shared_ptr<BPlusTreeIO> io = make_shared<BPlusTreeIO>();

		return io->open(_io, false);
	}
	ErrorInfo open(const shared_ptr<BPlusTreeIOOperation> &_io, bool reset)
	{
		GuardWrite locker(rwmutex);

		io = _io;

		if (io == NULL)
			return ErrorInfo(Error_Code_Param);

		/* load index boot information */
		ErrorInfo err = _get_boot_info();
		if (err && !reset)
			return err;

		if (err && reset)
		{
			err = _set_boot_info();
			if (err)
				return err;
		}

		/* set order and entries */
		order = (MAXBLOCKSIZE - sizeof(struct bplus_node)) / (sizeof(Key) + sizeof(off_t));
		entries = (MAXBLOCKSIZE - sizeof(struct bplus_node)) / (sizeof(Key) + sizeof(Data));

		/* init free node caches */
		caches = new char[MAXBLOCKSIZE * MIN_CACHE_NUM];

		return ErrorInfo();
	}
	ErrorInfo close()
	{
		GuardWrite locker(rwmutex);

		if (caches)
		{
			SAFE_DELETEARRAY(caches);
		}

		io = NULL;

		return ErrorInfo();
	}

	ErrorInfo set(const Key &key, const Data &data)
	{
		GuardWrite locker(rwmutex);

		if (io == NULL)
			return ErrorInfo(Error_Code_Fail);

		int ret = _bplus_tree_insert(key, data);
		if (ret == -1)
		{
			return ErrorInfo(Error_Code_Fail);
		}

		_set_boot_info();

		return ErrorInfo();
	}

	ErrorInfo set(const std::map<Key, Data> &datas)
	{
		GuardWrite locker(rwmutex);

		if (io == NULL)
			return ErrorInfo(Error_Code_Fail);

		for (typename std::map<Key, Data>::const_iterator iter = datas.begin(); iter != datas.end(); iter++)
		{
			int ret = _bplus_tree_insert(iter->first, iter->second);
			if (ret == -1)
			{
				return ErrorInfo(Error_Code_Fail);
			}
		}

		_set_boot_info();

		return ErrorInfo();
	}

	ErrorInfo del(const Key &key)
	{
		GuardWrite locker(rwmutex);

		if (io == NULL)
			return ErrorInfo(Error_Code_Fail);

		_bplus_tree_delete(key);

		_set_boot_info();

		return ErrorInfo();
	}

	ErrorInfo get(const Key &key, Data &data)
	{
		GuardRead locker(rwmutex);

		if (io == NULL)
			return ErrorInfo(Error_Code_Fail);

		bool haveFind = false;
		struct bplus_node *node = _node_seek(this->boot.root);
		while (node != NULL)
		{
			int i = _key_binary_search(node, key);
			if (_is_leaf(node))
			{
				if (i >= 0)
				{
					data = _data_ptr(node)[i];
					haveFind = true;
				}
				break;
			}
			else
			{
				if (i >= 0)
				{
					node = _node_seek(_sub_ptr(node)[i + 1]);
				}
				else
				{
					i = -i - 1;
					node = _node_seek(_sub_ptr(node)[i]);
				}
			}
		}

		if (!haveFind)
			return ErrorInfo(Error_Code_Fail);

		return ErrorInfo();
	}
	//搜索KEY1-KEY2之间的结果
	ErrorInfo search(const Key &key1, const Key &key2, std::list<Data> &datas)
	{
		GuardRead locker(rwmutex);

		const Key &min = key1 < key2 ? key1 : key2;
		const Key &max = min == key1 ? key2 : key1;

		if (io == NULL)
			return ErrorInfo(Error_Code_Fail);

		struct bplus_node *node = _node_seek(this->boot.root);
		while (node != NULL)
		{
			int i = _key_binary_search(node, min);
			if (_is_leaf(node))
			{
				if (i < 0)
				{
					i = -i - 1;
					if (i >= node->children)
					{
						node = _node_seek(node->next);
					}
				}
				while (node != NULL && (_key_ptr(node)[i] < max || _key_ptr(node)[i] == max))
				{
					Data data = _data_ptr(node)[i];
					datas.push_back(data);
					if (++i >= node->children)
					{
						node = _node_seek(node->next);
						i = 0;
					}
				}
				break;
			}
			else
			{
				if (i >= 0)
				{
					node = _node_seek(_sub_ptr(node)[i + 1]);
				}
				else
				{
					i = -i - 1;
					node = _node_seek(_sub_ptr(node)[i]);
				}
			}
		}

		return ErrorInfo();
	}

	//遍历树
	ErrorInfo foreachAll(const Function<void(const Key &key, const Data &data)> &func)
	{
		Function<void(const Key &key, const Data &data)> foreachfunc = func;

		GuardRead locker(rwmutex);

		struct bplus_node *node = _node_seek(this->boot.root);
		struct node_backlog *p_nbl = NULL;
		struct node_backlog nbl_stack[MAX_LEVEL];
		struct node_backlog *top = nbl_stack;

		for (;;)
		{
			if (node != NULL)
			{
				/* non-zero needs backward and zero does not */
				int sub_idx = p_nbl != NULL ? p_nbl->next_sub_idx : 0;
				/* Reset each loop */
				p_nbl = NULL;

				/* Backlog the node */
				if (_is_leaf(node) || sub_idx + 1 >= _children(node))
				{
					top->offset = INVALID_OFFSET;
					top->next_sub_idx = 0;
				}
				else
				{
					top->offset = node->self;
					top->next_sub_idx = sub_idx + 1;
				}
				top++;
				boot.level++;

				/* Draw the node when first passed through */
				if (sub_idx == 0)
				{
					_draw(node, nbl_stack, boot.level, foreachfunc);
				}

				/* Move deep down */
				node = _is_leaf(node) ? NULL : _node_seek(_sub_ptr(node)[sub_idx]);
			}
			else
			{
				p_nbl = top == nbl_stack ? NULL : --top;
				if (p_nbl == NULL)
				{
					/* End of traversal */
					break;
				}
				node = _node_seek(p_nbl->offset);
				boot.level--;
			}
		}

		return ErrorInfo();
	}

private:
	enum
	{
		BPLUS_TREE_LEAF,
		BPLUS_TREE_NON_LEAF = 1,
	};

	enum
	{
		LEFT_SIBLING,
		RIGHT_SIBLING = 1,
	};

	struct bplus_info
	{
		uint32_t block_count = 0;
		uint32_t level = 0;
		off_t root = INVALID_OFFSET;
		size_t file_size = 0;
		uint32_t total = 0;
	};

	struct bplus_node
	{
		off_t self;
		off_t parent;
		off_t prev;
		off_t next;
		uint32_t type;
		uint32_t children;
		uint32_t maxOrder;
		uint32_t maxEntries;
		uint32_t reserve;
	};

	struct node_backlog
	{
		/* Node backlogged */
		off_t offset;
		/* The index next to the backtrack point, must be >= 1 */
		int next_sub_idx;
	};

private:
	ReadWriteMutex rwmutex;

	shared_ptr<BPlusTreeIOOperation> io;

	char *caches = NULL;
	int used[MIN_CACHE_NUM];
	int order = 0;
	int entries = 0;

	struct bplus_info boot;
	std::list<off_t> free_blocks;

private:
	ErrorInfo _set_boot_info()
	{
		uint32_t offset = 0;

		ErrorInfo err = io->write(BPlusTreeIOOperation::IOType_BOOT, offset, &boot, sizeof(boot));
		if (err)
			return err;

		offset += sizeof(boot);
		for (std::list<off_t>::iterator iter = free_blocks.begin(); iter != free_blocks.end(); iter++)
		{
			off_t &offval = *iter;

			err = io->write(BPlusTreeIOOperation::IOType_BOOT, offset, &offval, sizeof(offval));
			if (err)
				return err;

			offset += sizeof(offval);
		}

		//写一个free_block的end
		{
			off_t offval = INVALID_OFFSET;
			err = io->write(BPlusTreeIOOperation::IOType_BOOT, offset, &offval, sizeof(offval));
			if (err)
				return err;
		}

		return ErrorInfo();
	}
	ErrorInfo _get_boot_info()
	{
		uint32_t offset = 0;

		ErrorInfo err = io->read(BPlusTreeIOOperation::IOType_BOOT, offset, &boot, sizeof(boot));
		if (err)
			return err;

		offset += sizeof(boot);

		while (1)
		{
			off_t offval = INVALID_OFFSET;
			err = io->read(BPlusTreeIOOperation::IOType_BOOT, offset, &offval, sizeof(offval));
			if (err || offval == INVALID_OFFSET)
				break;

			free_blocks.push_back(offval);

			offset += sizeof(offval);
		}

		return ErrorInfo();
	}

	bplus_node *_node_fetch(off_t offset)
	{
		if (offset == INVALID_OFFSET)
			return NULL;

		struct bplus_node *node = _cache_refer();

		ErrorInfo err = io->read(BPlusTreeIOOperation::IOType_TREE, offset, node, MAXBLOCKSIZE);
		if (err)
		{
			_cache_defer(node);
			node = NULL;
		}
		return node;
	}

	struct bplus_node *_node_seek(off_t offset)
	{
		if (offset == INVALID_OFFSET)
			return NULL;

		for (int i = 0; i < MIN_CACHE_NUM; i++)
		{
			if (!this->used[i])
			{
				char *buf = this->caches + MAXBLOCKSIZE * i;

				ErrorInfo err = io->read(BPlusTreeIOOperation::IOType_TREE, offset, buf, MAXBLOCKSIZE);
				if (err)
				{
					logdebug("len != MAXBLOCKSIZE with %llx\n", offset);
					return NULL;
				}
				return (struct bplus_node *)buf;
			}
		}
		assert(0);

		return NULL;
	}

	void _node_flush(struct bplus_node *node)
	{
		if (node != NULL)
		{
			ErrorInfo err = io->write(BPlusTreeIOOperation::IOType_TREE, node->self, node, MAXBLOCKSIZE);
			if (err)
			{
				logdebug("len != MAXBLOCKSIZE with %llx\n", node->self);
			}
			_cache_defer(node);
		}
	}

	bool _is_leaf(struct bplus_node *node)
	{
		return node->type == BPLUS_TREE_LEAF;
	}

	int _key_binary_search(struct bplus_node *node, const Key &target)
	{
		Key *arr = _key_ptr(node);
		//    printf("=====lenode->countn = %d\n", node->count);
		int len = _is_leaf(node) ? node->children : node->children - 1;
		int low = -1;
		int high = len;
		//    printf("=====len = %d\n", len);
		while (low + 1 < high)
		{
			int mid = low + (high - low) / 2;
			if (arr[mid] < target)
				low = mid;
			else
				high = mid;
		}

		if (high >= len || !(arr[high] == target))
			return -high - 1;
		else
			return high;
	}

	int _parent_key_index(struct bplus_node *parent, const Key &key)
	{
		int index = _key_binary_search(parent, key);
		return index >= 0 ? index : -index - 2;
	}

	struct bplus_node *_cache_refer()
	{
		for (int i = 0; i < MIN_CACHE_NUM; i++)
		{
			if (!this->used[i])
			{
				this->used[i] = 1;
				char *buf = this->caches + MAXBLOCKSIZE * i;
				memset(buf, 0, MAXBLOCKSIZE);
				return (struct bplus_node *)buf;
			}
		}
		assert(0);

		return NULL;
	}

	void _cache_defer(struct bplus_node *node)
	{
		/* return the node cache borrowed from */
		char *buf = (char *)node;
		size_t i = (buf - this->caches) / MAXBLOCKSIZE;
		this->used[i] = 0;
	}

	struct bplus_node *_node_new()
	{
		struct bplus_node *node = _cache_refer();
		node->prev = INVALID_OFFSET;
		node->next = INVALID_OFFSET;
		node->self = INVALID_OFFSET;
		node->parent = INVALID_OFFSET;
		node->children = 0;
		node->maxOrder = this->order;
		node->maxEntries = this->entries;
		this->boot.block_count++;

		return node;
	}

	struct bplus_node *_non_leaf_new()
	{
		struct bplus_node *node = _node_new();
		node->type = BPLUS_TREE_NON_LEAF;
		return node;
	}

	struct bplus_node *_leaf_new()
	{
		struct bplus_node *node = _node_new();
		node->type = BPLUS_TREE_LEAF;
		return node;
	}

	off_t _new_node_append(struct bplus_node *node)
	{
		/* assign new offset to the new node */
		if (this->free_blocks.size() == 0)
		{
			node->self = this->boot.file_size;
			this->boot.file_size += MAXBLOCKSIZE;
		}
		else
		{
			node->self = this->free_blocks.front();
			this->free_blocks.pop_front();
		}
		return node->self;
	}

	void _node_delete(struct bplus_node *node, struct bplus_node *left, struct bplus_node *right)
	{
		if (left != NULL)
		{
			if (right != NULL)
			{
				left->next = right->self;
				right->prev = left->self;
				_node_flush(right);
			}
			else
			{
				left->next = INVALID_OFFSET;
			}
			_node_flush(left);
		}
		else
		{
			if (right != NULL)
			{
				right->prev = INVALID_OFFSET;
				_node_flush(right);
			}
		}

		/* deleted blocks can be allocated for other nodes */
		this->free_blocks.push_back(node->self);
		this->boot.block_count--;

		/* return the cache borrowed from */
		_cache_defer(node);
	}

	void _sub_node_update(struct bplus_node *parent, int index, struct bplus_node *sub_node)
	{
		assert(sub_node->self != INVALID_OFFSET);
		_sub_ptr(parent)[index] = sub_node->self;
		sub_node->parent = parent->self;
		_node_flush(sub_node);
	}

	void _sub_node_flush(struct bplus_node *parent, off_t sub_offset)
	{
		struct bplus_node *sub_node = _node_fetch(sub_offset);
		assert(sub_node != NULL);
		sub_node->parent = parent->self;
		_node_flush(sub_node);
	}

	int _bplus_tree_search(const Key &key, off_t *offset)
	{
		struct bplus_node *node = _node_seek(this->boot.root);
		int i = -1;

		while (node != NULL)
		{
			i = _key_binary_search(node, key);
			if (_is_leaf(node))
			{
				if (i < 0)
				{
					i = -i - 1;
				}
				if (i == node->children)
				{
					*offset = node->next;
					i = 0;
				}
				else
				{
					*offset = node->self;
				}
				break;
			}
			else
			{
				if (i >= 0)
				{
					node = _node_seek(_sub_ptr(node)[i + 1]);
				}
				else
				{
					i = -i - 1;
					node = _node_seek(_sub_ptr(node)[i]);
				}
			}
		}
		return i;
	}

	void _left_node_add(struct bplus_node *node, struct bplus_node *left)
	{
		_new_node_append(left);

		struct bplus_node *prev = _node_fetch(node->prev);
		if (prev != NULL)
		{
			prev->next = left->self;
			left->prev = prev->self;
			_node_flush(prev);
		}
		else
		{
			left->prev = INVALID_OFFSET;
		}
		left->next = node->self;
		node->prev = left->self;
	}

	void _right_node_add(struct bplus_node *node, struct bplus_node *right)
	{
		_new_node_append(right);

		struct bplus_node *next = _node_fetch(node->next);
		if (next != NULL)
		{
			next->prev = right->self;
			right->next = next->self;
			_node_flush(next);
		}
		else
		{
			right->next = INVALID_OFFSET;
		}
		right->prev = node->self;
		node->next = right->self;
	}

	int _parent_node_build(struct bplus_node *l_ch, struct bplus_node *r_ch, const Key &key)
	{
		if (l_ch->parent == INVALID_OFFSET && r_ch->parent == INVALID_OFFSET)
		{
			/* new parent */
			struct bplus_node *parent = _non_leaf_new();
			_key_ptr(parent)[0] = key;
			_sub_ptr(parent)[0] = l_ch->self;
			_sub_ptr(parent)[1] = r_ch->self;
			parent->children = 2;
			/* write new parent and update root */
			this->boot.root = _new_node_append(parent);
			l_ch->parent = parent->self;
			r_ch->parent = parent->self;
			this->boot.level++;
			/* flush parent, left and right child */
			_node_flush(l_ch);
			_node_flush(r_ch);
			_node_flush(parent);
			return 0;
		}
		else if (r_ch->parent == INVALID_OFFSET)
		{
			return _non_leaf_insert(_node_fetch(l_ch->parent), l_ch, r_ch, key);
		}
		else
		{
			return _non_leaf_insert(_node_fetch(r_ch->parent), l_ch, r_ch, key);
		}
	}

	void _non_leaf_split_left(struct bplus_node *node, struct bplus_node *left, struct bplus_node *l_ch, struct bplus_node *r_ch, const Key &key, int insert, Key &split_key)
	{
		/* split = [m/2] */
		int split = (this->order + 1) / 2;

		/* split as left sibling */
		_left_node_add(node, left);

		/* calculate split nodes' children (sum as (order + 1))*/
		int pivot = insert;
		left->children = split;
		node->children = this->order - split + 1;

		/* sum = left->children = pivot + (split - pivot - 1) + 1 */
		/* replicate from key[0] to key[insert] in original node */
		memmove(&_key_ptr(left)[0], &_key_ptr(node)[0], pivot * sizeof(Key));
		memmove(&_sub_ptr(left)[0], &_sub_ptr(node)[0], pivot * sizeof(off_t));

		/* replicate from key[insert] to key[split - 1] in original node */
		memmove(&_key_ptr(left)[pivot + 1], &_key_ptr(node)[pivot], (split - pivot - 1) * sizeof(Key));
		memmove(&_sub_ptr(left)[pivot + 1], &_sub_ptr(node)[pivot], (split - pivot - 1) * sizeof(off_t));

		/* flush sub-nodes of the new splitted left node */
		for (int i = 0; i < left->children; i++)
		{
			if (i != pivot && i != pivot + 1)
			{
				_sub_node_flush(left, _sub_ptr(left)[i]);
			}
		}

		/* insert new key and sub-nodes and locate the split key */
		_key_ptr(left)[pivot] = key;
		if (pivot == split - 1)
		{
			/* left child in split left node and right child in original right one */
			_sub_node_update(left, pivot, l_ch);
			_sub_node_update(node, 0, r_ch);
			split_key = key;
		}
		else
		{
			/* both new children in split left node */
			_sub_node_update(left, pivot, l_ch);
			_sub_node_update(left, pivot + 1, r_ch);
			_sub_ptr(node)[0] = _sub_ptr(node)[split - 1];
			split_key = _key_ptr(node)[split - 2];
		}

		/* sum = node->children = 1 + (node->children - 1) */
		/* right node left shift from key[split - 1] to key[children - 2] */
		memmove(&_key_ptr(node)[0], &_key_ptr(node)[split - 1], (node->children - 1) * sizeof(Key));
		memmove(&_sub_ptr(node)[1], &_sub_ptr(node)[split], (node->children - 1) * sizeof(off_t));
	}

	void _non_leaf_split_right1(struct bplus_node *node, struct bplus_node *right, struct bplus_node *l_ch, struct bplus_node *r_ch, const Key &key, int insert, Key &split_key)
	{
		/* split = [m/2] */
		int split = (this->order + 1) / 2;

		/* split as right sibling */
		_right_node_add(node, right);

		/* split key is key[split - 1] */
		split_key = _key_ptr(node)[split - 1];

		/* calculate split nodes' children (sum as (order + 1))*/
		int pivot = 0;
		node->children = split;
		right->children = this->order - split + 1;

		/* insert new key and sub-nodes */
		_key_ptr(right)[0] = key;
		_sub_node_update(right, pivot, l_ch);
		_sub_node_update(right, pivot + 1, r_ch);

		/* sum = right->children = 2 + (right->children - 2) */
		/* replicate from key[split] to key[_max_order - 2] */
		memmove(&_key_ptr(right)[pivot + 1], &_key_ptr(node)[split], (right->children - 2) * sizeof(Key));
		memmove(&_sub_ptr(right)[pivot + 2], &_sub_ptr(node)[split + 1], (right->children - 2) * sizeof(off_t));

		/* flush sub-nodes of the new splitted right node */
		for (int i = pivot + 2; i < right->children; i++)
		{
			_sub_node_flush(right, _sub_ptr(right)[i]);
		}
	}

	void _non_leaf_split_right2(struct bplus_node *node, struct bplus_node *right, struct bplus_node *l_ch, struct bplus_node *r_ch, const Key &key, int insert, Key &split_key)
	{
		/* split = [m/2] */
		int split = (this->order + 1) / 2;

		/* split as right sibling */
		_right_node_add(node, right);

		/* split key is key[split] */
		split_key = _key_ptr(node)[split];

		/* calculate split nodes' children (sum as (order + 1))*/
		int pivot = insert - split - 1;
		node->children = split + 1;
		right->children = this->order - split;

		/* sum = right->children = pivot + 2 + (_max_order - insert - 1) */
		/* replicate from key[split + 1] to key[insert] */
		memmove(&_key_ptr(right)[0], &_key_ptr(node)[split + 1], pivot * sizeof(Key));
		memmove(&_sub_ptr(right)[0], &_sub_ptr(node)[split + 1], pivot * sizeof(off_t));

		/* insert new key and sub-node */
		_key_ptr(right)[pivot] = key;
		_sub_node_update(right, pivot, l_ch);
		_sub_node_update(right, pivot + 1, r_ch);

		/* replicate from key[insert] to key[order - 1] */
		memmove(&_key_ptr(right)[pivot + 1], &_key_ptr(node)[insert], (this->order - insert - 1) * sizeof(Key));
		memmove(&_sub_ptr(right)[pivot + 2], &_sub_ptr(node)[insert + 1], (this->order - insert - 1) * sizeof(off_t));

		/* flush sub-nodes of the new splitted right node */
		for (int i = 0; i < right->children; i++)
		{
			if (i != pivot && i != pivot + 1)
			{
				_sub_node_flush(right, _sub_ptr(right)[i]);
			}
		}
	}

	void _non_leaf_simple_insert(struct bplus_node *node, struct bplus_node *l_ch, struct bplus_node *r_ch, const Key &key, int insert)
	{
		memmove(&_key_ptr(node)[insert + 1], &_key_ptr(node)[insert], (node->children - 1 - insert) * sizeof(Key));
		memmove(&_sub_ptr(node)[insert + 2], &_sub_ptr(node)[insert + 1], (node->children - 1 - insert) * sizeof(off_t));
		/* insert new key and sub-nodes */
		_key_ptr(node)[insert] = key;
		_sub_node_update(node, insert, l_ch);
		_sub_node_update(node, insert + 1, r_ch);
		node->children++;
	}

	int _non_leaf_insert(struct bplus_node *node, struct bplus_node *l_ch, struct bplus_node *r_ch, const Key &key)
	{
		/* Search key location */
		int insert = _key_binary_search(node, key);
		//    assert(insert < 0);
		if (insert >= 0)
			return -1;
		insert = -insert - 1;

		/* node is full */
		if (node->children == this->order)
		{
			Key split_key;
			/* split = [m/2] */
			int split = (node->children + 1) / 2;
			struct bplus_node *sibling = _non_leaf_new();
			if (insert < split)
			{
				_non_leaf_split_left(node, sibling, l_ch, r_ch, key, insert, split_key);
			}
			else if (insert == split)
			{
				_non_leaf_split_right1(node, sibling, l_ch, r_ch, key, insert, split_key);
			}
			else
			{
				_non_leaf_split_right2(node, sibling, l_ch, r_ch, key, insert, split_key);
			}
			/* build new parent */
			if (insert < split)
			{
				return _parent_node_build(sibling, node, split_key);
			}
			else
			{
				return _parent_node_build(node, sibling, split_key);
			}
		}
		else
		{
			_non_leaf_simple_insert(node, l_ch, r_ch, key, insert);
			_node_flush(node);
		}
		return 0;
	}

	void _leaf_split_left(struct bplus_node *leaf, struct bplus_node *left, const Key &key, const Data &data, int insert, Key &split_key)
	{
		/* split = [m/2] */
		int split = (leaf->children + 1) / 2;

		/* split as left sibling */
		_left_node_add(leaf, left);

		/* calculate split leaves' children (sum as (entries + 1)) */
		int pivot = insert;
		left->children = split;
		leaf->children = this->entries - split + 1;

		/* sum = left->children = pivot + 1 + (split - pivot - 1) */
		/* replicate from key[0] to key[insert] */
		memmove(&_key_ptr(left)[0], &_key_ptr(leaf)[0], pivot * sizeof(Key));
		memmove(&_data_ptr(left)[0], &_data_ptr(leaf)[0], pivot * sizeof(Data));

		/* insert new key and data */
		_key_ptr(left)[pivot] = key;
		_data_ptr(left)[pivot] = data;

		/* replicate from key[insert] to key[split - 1] */
		memmove(&_key_ptr(left)[pivot + 1], &_key_ptr(leaf)[pivot], (split - pivot - 1) * sizeof(Key));
		memmove(&_data_ptr(left)[pivot + 1], &_data_ptr(leaf)[pivot], (split - pivot - 1) * sizeof(Data));

		/* original leaf left shift */
		memmove(&_key_ptr(leaf)[0], &_key_ptr(leaf)[split - 1], leaf->children * sizeof(Key));
		memmove(&_data_ptr(leaf)[0], &_data_ptr(leaf)[split - 1], leaf->children * sizeof(Data));

		split_key = _key_ptr(leaf)[0];
	}

	void _leaf_split_right(struct bplus_node *leaf, struct bplus_node *right, const Key &key, const Data &data, int insert, Key &split_key)
	{
		/* split = [m/2] */
		int split = (leaf->children + 1) / 2;

		/* split as right sibling */
		_right_node_add(leaf, right);

		/* calculate split leaves' children (sum as (entries + 1)) */
		int pivot = insert - split;
		leaf->children = split;
		right->children = this->entries - split + 1;

		/* sum = right->children = pivot + 1 + (_max_entries - pivot - split) */
		/* replicate from key[split] to key[children - 1] in original leaf */
		memmove(&_key_ptr(right)[0], &_key_ptr(leaf)[split], pivot * sizeof(Key));
		memmove(&_data_ptr(right)[0], &_data_ptr(leaf)[split], pivot * sizeof(Data));

		/* insert new key and data */
		_key_ptr(right)[pivot] = key;
		_data_ptr(right)[pivot] = data;

		/* replicate from key[insert] to key[children - 1] in original leaf */
		memmove(&_key_ptr(right)[pivot + 1], &_key_ptr(leaf)[insert], (this->entries - insert) * sizeof(Key));
		memmove(&_data_ptr(right)[pivot + 1], &_data_ptr(leaf)[insert], (this->entries - insert) * sizeof(Data));

		split_key = _key_ptr(right)[0];
	}

	void _leaf_simple_insert(struct bplus_node *leaf, const Key &key, const Data &data, int insert)
	{
		memmove(&_key_ptr(leaf)[insert + 1], &_key_ptr(leaf)[insert], (leaf->children - insert) * sizeof(Key));
		memmove(&_data_ptr(leaf)[insert + 1], &_data_ptr(leaf)[insert], (leaf->children - insert) * sizeof(Data));
		_key_ptr(leaf)[insert] = key;
		_data_ptr(leaf)[insert] = data;
		leaf->children++;
	}

	int _leaf_insert(struct bplus_node *leaf, const Key &key, const Data &data)
	{
		/* Search key location */
		int insert = _key_binary_search(leaf, key);
		if (insert >= 0)
		{
			/* Already exists */
			return -1;
		}
		insert = -insert - 1;

		/* fetch from free node caches */
		size_t i = ((char *)leaf - this->caches) / MAXBLOCKSIZE;
		this->used[i] = 1;

		/* leaf is full */
		if (leaf->children == this->entries)
		{
			Key split_key;
			/* split = [m/2] */
			int split = (this->entries + 1) / 2;
			struct bplus_node *sibling = _leaf_new();
			/* sibling leaf replication due to location of insertion */
			if (insert < split)
			{
				_leaf_split_left(leaf, sibling, key, data, insert, split_key);
			}
			else
			{
				_leaf_split_right(leaf, sibling, key, data, insert, split_key);
			}
			/* build new parent */
			if (insert < split)
			{
				return _parent_node_build(sibling, leaf, split_key);
			}
			else
			{
				return _parent_node_build(leaf, sibling, split_key);
			}
		}
		else
		{
			_leaf_simple_insert(leaf, key, data, insert);
			_node_flush(leaf);
		}

		return 0;
	}

	int _bplus_tree_insert(const Key &key, const Data &data)
	{
		struct bplus_node *node = _node_seek(this->boot.root);
		int res = -1;

		if (node == NULL)
		{
			/* new root */
			struct bplus_node *root = _leaf_new();
			_key_ptr(root)[0] = key;
			_data_ptr(root)[0] = data;
			root->children = 1;
			this->boot.root = _new_node_append(root);
			this->boot.level = 1;
			_node_flush(root);
			this->boot.total++;
			res = 0;
		}
		else
		{
			while (node != NULL)
			{
				if (_is_leaf(node))
				{
					res = _leaf_insert(node, key, data);
					if (res == 0)
					{
						this->boot.total++;
					}
					break;
				}
				else
				{
					int i = _key_binary_search(node, key);
					if (i >= 0)
					{
						node = _node_seek(_sub_ptr(node)[i + 1]);
					}
					else
					{
						i = -i - 1;
						node = _node_seek(_sub_ptr(node)[i]);
					}
				}
			}
		}

		return res;
	}

	int _sibling_select(struct bplus_node *l_sib, struct bplus_node *r_sib, struct bplus_node *parent, int i)
	{
		if (i == -1)
		{
			/* the frist sub-node, no left sibling, choose the right one */
			return RIGHT_SIBLING;
		}
		else if (i == parent->children - 2)
		{
			/* the last sub-node, no right sibling, choose the left one */
			return LEFT_SIBLING;
		}
		else
		{
			/* if both left and right sibling found, choose the one with more children */
			return l_sib->children >= r_sib->children ? LEFT_SIBLING : RIGHT_SIBLING;
		}
	}

	void _non_leaf_shift_from_left(struct bplus_node *node, struct bplus_node *left, struct bplus_node *parent, int parent_key_index, int remove)
	{
		/* node's elements right shift */
		memmove(&_key_ptr(node)[1], &_key_ptr(node)[0], remove * sizeof(Key));
		memmove(&_sub_ptr(node)[1], &_sub_ptr(node)[0], (remove + 1) * sizeof(off_t));

		/* parent key right rotation */
		_key_ptr(node)[0] = _key_ptr(parent)[parent_key_index];
		_key_ptr(parent)[parent_key_index] = _key_ptr(left)[left->children - 2];

		/* borrow the last sub-node from left sibling */
		_sub_ptr(node)[0] = _sub_ptr(left)[left->children - 1];
		_sub_node_flush(node, _sub_ptr(node)[0]);

		left->children--;
	}

	void _non_leaf_merge_into_left(struct bplus_node *node, struct bplus_node *left, struct bplus_node *parent, int parent_key_index, int remove)
	{
		/* move parent key down */
		_key_ptr(left)[left->children - 1] = _key_ptr(parent)[parent_key_index];

		/* merge into left sibling */
		/* key sum = node->children - 2 */
		memmove(&_key_ptr(left)[left->children], &_key_ptr(node)[0], remove * sizeof(Key));
		memmove(&_sub_ptr(left)[left->children], &_sub_ptr(node)[0], (remove + 1) * sizeof(off_t));

		/* sub-node sum = node->children - 1 */
		memmove(&_key_ptr(left)[left->children + remove], &_key_ptr(node)[remove + 1], (node->children - remove - 2) * sizeof(Key));
		memmove(&_sub_ptr(left)[left->children + remove + 1], &_sub_ptr(node)[remove + 2], (node->children - remove - 2) * sizeof(off_t));

		/* flush sub-nodes of the new merged left node */
		int i, j;
		for (i = left->children, j = 0; j < node->children - 1; i++, j++)
		{
			_sub_node_flush(left, _sub_ptr(left)[i]);
		}

		left->children += node->children - 1;
	}

	void _non_leaf_shift_from_right(struct bplus_node *node, struct bplus_node *right, struct bplus_node *parent, int parent_key_index)
	{
		/* parent key left rotation */
		_key_ptr(node)[node->children - 1] = _key_ptr(parent)[parent_key_index];
		_key_ptr(parent)[parent_key_index] = _key_ptr(right)[0];

		/* borrow the frist sub-node from right sibling */
		_sub_ptr(node)[node->children] = _sub_ptr(right)[0];
		_sub_node_flush(node, _sub_ptr(node)[node->children]);
		node->children++;

		/* right sibling left shift*/
		memmove(&_key_ptr(right)[0], &_key_ptr(right)[1], (right->children - 2) * sizeof(Key));
		memmove(&_sub_ptr(right)[0], &_sub_ptr(right)[1], (right->children - 1) * sizeof(off_t));

		right->children--;
	}

	void _non_leaf_merge_from_right(struct bplus_node *node, struct bplus_node *right, struct bplus_node *parent, int parent_key_index)
	{
		/* move parent key down */
		_key_ptr(node)[node->children - 1] = _key_ptr(parent)[parent_key_index];
		node->children++;

		/* merge from right sibling */
		memmove(&_key_ptr(node)[node->children - 1], &_key_ptr(right)[0], (right->children - 1) * sizeof(Key));
		memmove(&_sub_ptr(node)[node->children - 1], &_sub_ptr(right)[0], right->children * sizeof(off_t));

		/* flush sub-nodes of the new merged node */
		int i, j;
		for (i = node->children - 1, j = 0; j < right->children; i++, j++)
		{
			_sub_node_flush(node, _sub_ptr(node)[i]);
		}

		node->children += right->children - 1;
	}

	void _non_leaf_simple_remove(struct bplus_node *node, int remove)
	{
		assert(node->children >= 2);
		memmove(&_key_ptr(node)[remove], &_key_ptr(node)[remove + 1], (node->children - remove - 2) * sizeof(Key));
		memmove(&_sub_ptr(node)[remove + 1], &_sub_ptr(node)[remove + 2], (node->children - remove - 2) * sizeof(off_t));
		node->children--;
	}

	void _non_leaf_remove(struct bplus_node *node, int remove)
	{
		if (node->parent == INVALID_OFFSET)
		{
			/* node is the root */
			if (node->children == 2)
			{
				/* replace old root with the first sub-node */
				struct bplus_node *root = _node_fetch(_sub_ptr(node)[0]);
				root->parent = INVALID_OFFSET;
				this->boot.root = root->self;
				this->boot.level--;
				_node_delete(node, NULL, NULL);
				_node_flush(root);
			}
			else
			{
				_non_leaf_simple_remove(node, remove);
				_node_flush(node);
			}
		}
		else if (node->children <= (this->order + 1) / 2)
		{
			struct bplus_node *l_sib = _node_fetch(node->prev);
			struct bplus_node *r_sib = _node_fetch(node->next);
			struct bplus_node *parent = _node_fetch(node->parent);

			int i = _parent_key_index(parent, _key_ptr(node)[0]);

			/* decide which sibling to be borrowed from */
			if (_sibling_select(l_sib, r_sib, parent, i) == LEFT_SIBLING)
			{
				if (l_sib->children > (this->order + 1) / 2)
				{
					_non_leaf_shift_from_left(node, l_sib, parent, i, remove);
					/* flush nodes */
					_node_flush(node);
					_node_flush(l_sib);
					_node_flush(r_sib);
					_node_flush(parent);
				}
				else
				{
					_non_leaf_merge_into_left(node, l_sib, parent, i, remove);
					/* delete empty node and flush */
					_node_delete(node, l_sib, r_sib);
					/* trace upwards */
					_non_leaf_remove(parent, i);
				}
			}
			else
			{
				/* remove at first in case of overflow during merging with sibling */
				_non_leaf_simple_remove(node, remove);

				if (r_sib->children > (this->order + 1) / 2)
				{
					_non_leaf_shift_from_right(node, r_sib, parent, i + 1);
					/* flush nodes */
					_node_flush(node);
					_node_flush(l_sib);
					_node_flush(r_sib);
					_node_flush(parent);
				}
				else
				{
					_non_leaf_merge_from_right(node, r_sib, parent, i + 1);
					/* delete empty right sibling and flush */
					struct bplus_node *rr_sib = _node_fetch(r_sib->next);
					_node_delete(r_sib, node, rr_sib);
					_node_flush(l_sib);
					/* trace upwards */
					_non_leaf_remove(parent, i + 1);
				}
			}
		}
		else
		{
			_non_leaf_simple_remove(node, remove);
			_node_flush(node);
		}
	}

	void _leaf_shift_from_left(struct bplus_node *leaf, struct bplus_node *left, struct bplus_node *parent, int parent_key_index, int remove)
	{
		/* right shift in leaf node */
		memmove(&_key_ptr(leaf)[1], &_key_ptr(leaf)[0], remove * sizeof(Key));
		memmove(&_data_ptr(leaf)[1], &_data_ptr(leaf)[0], remove * sizeof(Data));

		/* borrow the last element from left sibling */
		_key_ptr(leaf)[0] = _key_ptr(left)[left->children - 1];
		_data_ptr(leaf)[0] = _data_ptr(left)[left->children - 1];
		left->children--;

		/* update parent key */
		_key_ptr(parent)[parent_key_index] = _key_ptr(leaf)[0];
	}

	void _leaf_merge_into_left(struct bplus_node *leaf, struct bplus_node *left, int parent_key_index, int remove)
	{
		/* merge into left sibling, sum = leaf->children - 1*/
		memmove(&_key_ptr(left)[left->children], &_key_ptr(leaf)[0], remove * sizeof(Key));
		memmove(&_data_ptr(left)[left->children], &_data_ptr(leaf)[0], remove * sizeof(Data));
		memmove(&_key_ptr(left)[left->children + remove], &_key_ptr(leaf)[remove + 1], (leaf->children - remove - 1) * sizeof(Key));
		memmove(&_data_ptr(left)[left->children + remove], &_data_ptr(leaf)[remove + 1], (leaf->children - remove - 1) * sizeof(Data));
		left->children += leaf->children - 1;
	}

	void _leaf_shift_from_right(struct bplus_node *leaf, struct bplus_node *right, struct bplus_node *parent, int parent_key_index)
	{
		/* borrow the first element from right sibling */
		_key_ptr(leaf)[leaf->children] = _key_ptr(right)[0];
		_data_ptr(leaf)[leaf->children] = _data_ptr(right)[0];
		leaf->children++;

		/* left shift in right sibling */
		memmove(&_key_ptr(right)[0], &_key_ptr(right)[1], (right->children - 1) * sizeof(Key));
		memmove(&_data_ptr(right)[0], &_data_ptr(right)[1], (right->children - 1) * sizeof(Data));
		right->children--;

		/* update parent key */
		_key_ptr(parent)[parent_key_index] = _key_ptr(right)[0];
	}

	void _leaf_merge_from_right(struct bplus_node *leaf, struct bplus_node *right)
	{
		memmove(&_key_ptr(leaf)[leaf->children], &_key_ptr(right)[0], right->children * sizeof(Key));
		memmove(&_data_ptr(leaf)[leaf->children], &_data_ptr(right)[0], right->children * sizeof(Data));
		leaf->children += right->children;
	}

	void _leaf_simple_remove(struct bplus_node *leaf, int remove)
	{
		memmove(&_key_ptr(leaf)[remove], &_key_ptr(leaf)[remove + 1], (leaf->children - remove - 1) * sizeof(Key));
		memmove(&_data_ptr(leaf)[remove], &_data_ptr(leaf)[remove + 1], (leaf->children - remove - 1) * sizeof(Data));
		leaf->children--;
	}

	int _leaf_remove(struct bplus_node *leaf, const Key &key)
	{
		int remove = _key_binary_search(leaf, key);
		if (remove < 0)
		{
			/* Not exist */
			return -1;
		}

		/* fetch from free node caches */
		size_t i = ((char *)leaf - this->caches) / MAXBLOCKSIZE;
		this->used[i] = 1;

		if (leaf->parent == INVALID_OFFSET)
		{
			/* leaf as the root */
			if (leaf->children == 1)
			{
				/* delete the only last node */
				assert(key == _key_ptr(leaf)[0]);
				this->boot.root = INVALID_OFFSET;
				this->boot.level = 0;
				_node_delete(leaf, NULL, NULL);
			}
			else
			{
				_leaf_simple_remove(leaf, remove);
				_node_flush(leaf);
			}
		}
		else if (leaf->children <= (this->entries + 1) / 2)
		{
			struct bplus_node *l_sib = _node_fetch(leaf->prev);
			struct bplus_node *r_sib = _node_fetch(leaf->next);
			struct bplus_node *parent = _node_fetch(leaf->parent);

			i = _parent_key_index(parent, _key_ptr(leaf)[0]);

			/* decide which sibling to be borrowed from */
			if (_sibling_select(l_sib, r_sib, parent, (int)i) == LEFT_SIBLING)
			{
				if (l_sib->children > (this->entries + 1) / 2)
				{
					_leaf_shift_from_left(leaf, l_sib, parent, (int)i, remove);
					/* flush leaves */
					_node_flush(leaf);
					_node_flush(l_sib);
					_node_flush(r_sib);
					_node_flush(parent);
				}
				else
				{
					_leaf_merge_into_left(leaf, l_sib, (int)i, remove);
					/* delete empty leaf and flush */
					_node_delete(leaf, l_sib, r_sib);
					/* trace upwards */
					_non_leaf_remove(parent, (int)i);
				}
			}
			else
			{
				/* remove at first in case of overflow during merging with sibling */
				_leaf_simple_remove(leaf, remove);

				if (r_sib->children > (this->entries + 1) / 2)
				{
					_leaf_shift_from_right(leaf, r_sib, parent, (int)i + 1);
					/* flush leaves */
					_node_flush(leaf);
					_node_flush(l_sib);
					_node_flush(r_sib);
					_node_flush(parent);
				}
				else
				{
					_leaf_merge_from_right(leaf, r_sib);
					/* delete empty right sibling flush */
					struct bplus_node *rr_sib = _node_fetch(r_sib->next);
					_node_delete(r_sib, leaf, rr_sib);
					_node_flush(l_sib);
					/* trace upwards */
					_non_leaf_remove(parent, (int)i + 1);
				}
			}
		}
		else
		{
			_leaf_simple_remove(leaf, remove);
			_node_flush(leaf);
		}

		return 0;
	}

	int _bplus_tree_delete(const Key &key)
	{
		struct bplus_node *node = _node_seek(this->boot.root);
		int res = -1;

		while (node != NULL)
		{
			if (_is_leaf(node))
			{
				res = _leaf_remove(node, key);
				if (res == 0)
					this->boot.total--;
				break;
			}
			else
			{
				int i = _key_binary_search(node, key);
				if (i >= 0)
				{
					node = _node_seek(_sub_ptr(node)[i + 1]);
				}
				else
				{
					i = -i - 1;
					node = _node_seek(_sub_ptr(node)[i]);
				}
			}
		}

		return res;
	}

	int _children(struct bplus_node *node)
	{
		assert(!_is_leaf(node));
		return node->children;
	}

	void _draw(struct bplus_node *node, struct node_backlog *stack, int level, Function<void(const Key &key, const Data &data)> &func)
	{
		if (_is_leaf(node))
		{
			for (int i = 0; i < node->children; i++)
			{
				func(_key_ptr(node)[i], _data_ptr(node)[i]);
			}
		}
		else
		{
			for (int i = 0; i < node->children - 1; i++)
			{
				func(_key_ptr(node)[i], _data_ptr(node)[i]);
			}
		}
	}
};

} // namespace Base
} // namespace Public