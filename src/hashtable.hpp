#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include <cstddef>
#include <cstring>

#include "skiplist.hpp"

template<size_t H>
struct hash_node_t {
	char key[H];
	char* val;
	size_t val_sz;
	hash_node_t<H>* next;

	hash_node_t<H>(char key[H], char* val, size_t val_sz, hash_node_t<H>* next);
	~hash_node_t<H>();
};

template<size_t H>
struct hashtable_t {
	hash_node_t<H>** table;
	size_t size;
	size_t log_sz;

	hashtable_t<H>(size_t log_sz);
	void add(char key[H], char* val, size_t val_sz);
	void del(char key[H]);
	char* get(char key[H], size_t* val_sz);

	void clean(char begin[H], char end[H]);
};

/************ impl ************/

// Note: val must have been new'ed
template<size_t H>
hash_node_t<H>::hash_node_t(
	char key[H],
	char* val,
	size_t val_sz,
	hash_node_t<H>* next)
{
	memcpy(this->key, key, H);
	this->val    = val;
	this->val_sz = val_sz;
	this->next   = next;
}

template<size_t H>
hash_node_t<H>::~hash_node_t()
{
	delete[] val;
}

template<size_t H>
hashtable_t<H>::hashtable_t(
	size_t log_sz)
{
	this->log_sz = log_sz;
	this->size = 0;
	size_t capacity = 1 << log_sz;
	this->table = new hash_node_t<H>*[capacity];
	memset(this->table, 0, capacity*sizeof(hash_node_t<H>*));
}

template<size_t H>
hash_node_t<H>** 
resize(
	hash_node_t<H>** table,
	size_t log_sz)
{
	size_t old_capacity = 1 << log_sz;

	hash_node_t<H>** ret = new hash_node_t<H>*[2*old_capacity];

	for (size_t i = 0; i < old_capacity; ++i) {
		hash_node_t<H>* rd  = table[i];
		hash_node_t<H>** wr0 = ret + i;
		hash_node_t<H>** wr1 = ret + old_capacity + i;

		*wr0 = nullptr;
		*wr1 = nullptr;

		while (rd != nullptr) {
			size_t ind = *((size_t*)rd->key);

			if ((ind & old_capacity) != 0) {
				*wr1 = rd;
				wr1 = &(rd->next);
			}
			else {
				*wr0 = rd;
				wr0 = &(rd->next);
			}
			rd = rd->next;
		}
	}

	delete[] table;

	return ret;
}

template<size_t H>
void
hashtable_t<H>::add(
	char key[H],
	char* val,
	size_t val_sz)
{
	if (val == nullptr || val_sz == 0) {
		del(key);
		return;
	}
	size_t capacity = 1 << log_sz;
	if (size >= capacity * 3/4) {
		table = resize<H>(table, log_sz);
		log_sz += 1;
		capacity = 1 << log_sz;
	}

	size_t ind = *((size_t*)key);
	ind = ind & (capacity-1);

	hash_node_t<H>* cur_node = table[ind];
	if (cur_node == nullptr) {
		table[ind] = new hash_node_t<H>(key, val, val_sz, nullptr);
		size += 1;
		return;
	}
	else {
		while (memcmp(cur_node->key, key, H) != 0) {
			if (cur_node->next == nullptr) {
				cur_node->next = new hash_node_t<H>(key, val, val_sz, nullptr);
				size += 1;
				return;
			}
			cur_node = cur_node->next;
		}

		// overwrite old
		delete[] cur_node->val;
		cur_node->val = val;
		cur_node->val_sz = val_sz;
	}
}

template<size_t H>
void
hashtable_t<H>::del(
	char key[H])
{
	size_t capacity = 1 << log_sz;
	size_t ind = *((size_t*)key);
	ind = ind & (capacity-1);

	hash_node_t<H>* cur_node = table[ind];
	if (cur_node == nullptr) {
		return;
	}
	else {
		hash_node_t<H>** prev_next = &(table[ind]);
		while (memcmp(cur_node->key, key, H) != 0) {
			if (cur_node->next == nullptr) {
				return;
			}
			prev_next = &(cur_node->next);
			cur_node = cur_node->next;
		}
		*prev_next = cur_node->next;
		delete cur_node;
		size -= 1;
	}
}


template<size_t H>
char*
hashtable_t<H>::get(
	char key[H],
	size_t* val_sz)
{
	size_t capacity = 1 << log_sz;
	size_t ind = *((size_t*)key);
	ind = ind & (capacity-1);

	hash_node_t<H>* cur_node = table[ind];
	if (cur_node == nullptr) {
		return nullptr;
	}
	else {
		while (memcmp(cur_node->key, key, H) != 0) {
			if (cur_node->next == nullptr) {
				return nullptr;
			}
			cur_node = cur_node->next;
		}
		*val_sz = cur_node->val_sz;
		return cur_node->val;
	}
}

template<size_t H>
void
hashtable_t<H>::clean(
	char begin[H],
	char end[H])
{
	if (memcmp(begin, end, H) == 0)
		return;
	size_t capacity = 1 << log_sz;
	for (int i = 0; i < (int) capacity; ++i) {
		hash_node_t<H>* cur_node = table[i];
		hash_node_t<H>** prev_next = &(table[i]);
		while (cur_node != nullptr) {
			if (!in_range<H>(begin, end, cur_node->key)) {
				// delete
				*prev_next = cur_node->next;
				delete cur_node;
				cur_node = *prev_next;
				size -= 1;
			}
			else {
				cur_node = cur_node->next;
			}
		}
	}
}
#endif
