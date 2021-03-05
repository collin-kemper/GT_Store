#include "hash/blake2.h"

#include "gtstore.hpp"

#define MAX_EPOLL_EVENTS 16

GTStoreStorage::GTStoreStorage()
	: htable(hashtable_t<HASHSZ>(10)) {

	pid_t pid = getpid();
	blake2b(hash_id, HASHSZ, &pid, sizeof(pid), 0, 0);
}

void GTStoreStorage::init() {
	// init hash
	string name = hash_to_storage_name<HASHSZ>(hash_id);

	cout << "initing storage node with name "
		<< name << "\n";

	recv_msg_fd = recv_msg_socket(name);

	send_join(recv_msg_fd, MANAGER_MSG, hash_id);

	cout << "\tsending init message to manager\n";
	cout << "done initing storage\n";
}

void
GTStoreStorage::send_data_range(
	char dst_id[HASHSZ],
	char begin[HASHSZ],
	char end[HASHSZ])
{
	string dst_name = hash_to_storage_name<HASHSZ>(dst_id);

	// stream data in range (begin, end] to node with id dst_id
	size_t capacity = 1<< htable.log_sz;
	for (int i = 0; i < (int) capacity; ++i) {
		hash_node_t<HASHSZ>* cur_node = htable.table[i];
		while (cur_node != nullptr) {
			if (in_range<HASHSZ>(begin, end, cur_node->key)) {
				// for key, val, in hashtable
				send_put(recv_msg_fd, dst_name, cur_node->key, cur_node->val, cur_node->val_sz);
			}
			cur_node = cur_node->next;
		}
	}
}

void
GTStoreStorage::handle_join(
	char peer_id[HASHSZ])
{
	cout << "\tjoined by peer " << hash_to_string<HASHSZ>(peer_id) << "\n";
	cout << "\tsize before: " << htable.size << "\n";
	if(in_range<HASHSZ>(hash_id, succ, peer_id)) {
		cout << "\t\tcase 2:\n";
		// if peer is succ, stream them data in range (pred1, hash_id]
		send_data_range(peer_id, pred1, hash_id);
		// then make succ <- peer_id
		memcpy(succ, peer_id, HASHSZ);
	}
	if(in_range<HASHSZ>(pred1, hash_id, peer_id)) {
		cout << "\t\tcase 1:\n";
		// if peer is immediate pred, stream them data in range pred0, peer_id
		send_data_range(peer_id, pred0, peer_id);
		// then make pred0 <- pred1, pred1 <- peer_id
		memcpy(pred0, pred1, HASHSZ);
		memcpy(pred1, peer_id, HASHSZ);
		// then remove data not in range (pred0, hash]
		htable.clean(pred0, hash_id);
		if(in_range<HASHSZ>(hash_id, succ, peer_id)) {
			memcpy(succ, peer_id, HASHSZ);
		}
	}
	else if (in_range<HASHSZ>(pred0, pred1, peer_id)) {
		cout << "\t\tcase 0:\n";
		// then make pred0 <- peer_id
		memcpy(pred0, peer_id, HASHSZ);
		// then remove data not in range (pred0, hash]
		htable.clean(pred0, hash_id);
	}
	cout << "\tsize after: " << htable.size << "\n";
	cout << "\tdone handling join\n";
}

void
GTStoreStorage::handle_get(
	char key[HASHSZ],
	char cli_id[HASHSZ])
{
	cout << "\tgetting key " << hash_to_string<HASHSZ>(key) << "\n";
	size_t val_sz;
	char* val = htable.get(key, &val_sz);
	if (val == nullptr) {
		val_sz = 0;
	}

	cout << "\tval_sz = " << val_sz << "\n";
	cout << "\tval: " << val << "\n";

	string dst_name = hash_to_client_name<HASHSZ>(cli_id);
	send_put(recv_msg_fd, dst_name, key, val, val_sz);

	cout << "\tdone streaming value\n";
}

void
GTStoreStorage::handle_msg(
	int fd)
{
	cout << "msg event: ";
	storage_msg msg;
	int len = recv(fd, &msg, sizeof(msg), 0);
	if (msg.type == STORAGE_MSG_PUT) {
		cout << "put\n";
		// handle
		char* val = nullptr;

		val = new char[1024];
		memcpy(val, &(msg.msg.put.val), 1024);
		cout << "\tval: " << val << "\n";
		htable.add(msg.msg.put.key, val, 1024);

		cout << "\tcurrent size: " << htable.size << "\n";
	}
	else if (msg.type == STORAGE_MSG_JOIN) {
		cout << "join\n";
		handle_join(msg.msg.join.peer_id);
	}
	else if (msg.type == STORAGE_MSG_GET) {
		cout << "get\n";
		handle_get(msg.msg.get.hash, msg.msg.get.cli_id);
	}
	else if (msg.type == STORAGE_MSG_INIT) {
		cout << "init\n";
		memcpy(pred0, msg.msg.init.pred0, HASHSZ);
		memcpy(pred1, msg.msg.init.pred1, HASHSZ);
		memcpy(succ,  msg.msg.init.succ,  HASHSZ);
		cout << "\trecieved init_msg of length " << len << "\n";
		cout << "\tpeers are:\n";
		cout << "\t\t" << hash_to_string<HASHSZ>(pred0) <<  "\n";
		cout << "\t\t" << hash_to_string<HASHSZ>(pred1) <<  "\n";
		cout << "\t\t" << hash_to_string<HASHSZ>(succ) <<  "\n";
	}
	cout << "done handling msg event\n";
}

void
GTStoreStorage::mainloop()
{
	cout << "Inside GTStoreStorage main loop\n";

	while (true) {
		handle_msg(recv_msg_fd);
	}
}

int main()
{
	GTStoreStorage storage;
	storage.init();
	storage.mainloop();
}
