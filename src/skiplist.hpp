#ifndef __SKIPLIST_HPP
#define __SKIPLIST_HPP

#include <cstddef>
#include <string.h>

/**************** structs ****************/
// H = hash size in bytes
/*
template<size_t H>
struct node_info {
	char hash_id[H];
	uint64_t epoch;
};
*/

template<size_t H, size_t D>
struct skiplist_node {
	skiplist_node<H, D>* nexts[D];
	skiplist_node<H, D>* prevs[D];
	char hash[H];
	uint64_t accesses;
};

template<size_t H, size_t D>
struct skiplist {
	skiplist_node<H, D>* heads[D];

	skiplist<H,D>();

	void add(char key[H]);
	void del(char key[H]);
	skiplist_node<H,D>* succ(char key[H]);
};

/**************** helper functions ****************/
template<size_t H>
bool
hash_lt(
	char h0[H],
	char h1[H])
{
	for (int i = H-1; i >= 0; --i) {
		if (h0[i] < h1[i])
			return true;
		if (h0[i] > h1[i])
			return false;
	}
	return false;
}

template<size_t H>
bool
hash_le(
	char h0[H],
	char h1[H])
{
	for (int i = H-1; i >= 0; --i) {
		if (h0[i] < h1[i])
			return true;
		if (h0[i] > h1[i])
			return false;
	}
	return true;
}

template<size_t H>
bool
in_range(
	char begin[H],
	char end[H],
	char key[H])
{
	bool b_lt_k = hash_lt<H>(begin, key);
	bool b_lt_e = hash_lt<H>(begin, end);
	bool k_le_e = hash_le<H>(key, end);
	
	return (b_lt_k && k_le_e && b_lt_e)
		|| ((b_lt_k || k_le_e) && !b_lt_e);
}

template<size_t H>
size_t
get_height(
	char key[H])
{
	size_t ret = 0;
	for (int i = 0; i < (int) H; ++i) {
		for (int j = 0; j < 8; j += 2) {
			if (((key[i]>>j)&0x3) != 0) {
				return ret;
			}
			ret += 1;
		}
	}
	return ret;
}

/**************** impls ****************/
template<size_t H, size_t D>
skiplist<H,D>::skiplist()
{
	memset(heads, 0, sizeof(heads));
}

template<size_t H, size_t D>
void
skiplist<H,D>::add(
	char key[H])
{
	skiplist_node<H,D>* begin;
	skiplist_node<H,D>* end;
	skiplist_node<H,D>* begins[D];
	int highest;
	for (highest = D-1; highest>=0; --highest) {
		if (heads[highest] != nullptr) {
			begin = heads[highest];
			end = heads[highest]->nexts[highest];
			break;
		}
		begins[highest] = nullptr;
	}
	for (int i = highest; i >= 0; --i) {
		end = begin->nexts[i];
		while (!in_range<H>(begin->hash, end->hash, key)) {
			begin = end;
			end = end->nexts[i];
		}
		begins[i] = begin;
	}

	size_t height = get_height<H>(key);
	skiplist_node<H,D>* new_node = new skiplist_node<H, D>();
	memcpy(new_node->hash, key, H);
	for (size_t i = 0; i <= height && i < D; ++i) {
		if (heads[i] == nullptr) {
			heads[i] = new_node;
			new_node->nexts[i] = new_node;
			new_node->prevs[i] = new_node;
		}
		else {
			// begin
			new_node->nexts[i] = begins[i]->nexts[i];
			new_node->prevs[i] = begins[i];
			begins[i]->nexts[i]->prevs[i] = new_node;
			begins[i]->nexts[i] = new_node;
		}
	}
	for (int i = height+1; i < (int) D; ++i) {
		new_node->nexts[i] = nullptr;
		new_node->prevs[i] = nullptr;
	}
}

template<size_t H, size_t D>
void
skiplist<H,D>::del(
	char key[H])
{
	skiplist_node<H,D>* res = succ(key);

	if (memcmp(res->hash, key, H) != 0) {
		// does not contain key
		return;
	}

	for (int i = 0; i < D; ++i) {
		if (res->nexts[i] == nullptr) {
			break;
		}
		if (res->nexts[i] == res) {
			heads[i] == nullptr;
		}
		else {
			if (heads[i] == res) {
				heads[i] = res->nexts[i];
			}
			res->prevs[i]->nexts[i] = res->nexts[i];
			res->nexts[i]->prevs[i] = res->prevs[i];
		}
	}

	delete res;
}

template<size_t H, size_t D>
skiplist_node<H,D>*
skiplist<H,D>::succ(
	char key[H])
{
	skiplist_node<H,D>* begin = nullptr;
	skiplist_node<H,D>* end = nullptr;
	int highest;
	for (highest = D-1; highest>=0; --highest) {
		if (heads[highest] != nullptr) {
			begin = heads[highest];
			end = heads[highest]->nexts[highest];
			break;
		}
	}
	for (int i = highest; i >= 0; --i) {
		end = begin->nexts[i];
		while (!in_range<H>(begin->hash, end->hash, key)) {
			begin = end;
			end = end->nexts[i];
		}
	}

	return end;
}
#endif
