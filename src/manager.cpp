#include "gtstore.hpp"

#define MAX_EPOLL_EVENTS 16

void
GTStoreManager::init() {
	
	cout << "Inside GTStoreManager::init()\n";

	recv_msg_fd = recv_msg_socket(MANAGER_MSG);
}

void
GTStoreManager::handle_msg(
	int fd)
{
	cout << "msg event: ";
	storage_msg msg;
	recv(fd, &msg, sizeof(msg), 0);

	if (msg.type == STORAGE_MSG_PUT) {
		cout << "put\n";

		cout << "\tkey: " << hash_to_string<HASHSZ>(msg.msg.put.key) << "\n";
		cout << "\tval: " << msg.msg.put.val << "\n";
		auto snode = ring.succ(msg.msg.put.key);

		snode->accesses += 1;
		send_msg_to_storage(fd, snode->hash, msg);
		if (snode != snode->nexts[0]) {
			snode->nexts[0]->accesses += 1;
			send_msg_to_storage(fd, snode->nexts[0]->hash, msg);
		}
	}
	else if (msg.type == STORAGE_MSG_GET) {
		cout << "get\n";
		auto snode0 = ring.succ(msg.msg.get.hash);
		auto snode1 = snode0->nexts[0];

		if (snode0->accesses <= snode1->accesses) {
			snode0->accesses += 1;
			send_msg_to_storage(fd, snode0->hash, msg);
		}
		else {
			snode1->accesses += 1;
			send_msg_to_storage(fd, snode1->hash, msg);
		}
	}
	else if (msg.type == STORAGE_MSG_JOIN) {
		cout << "init storage for node "
			<< hash_to_string<HASHSZ>(msg.msg.join.peer_id) << "\n";

		ring.add(msg.msg.join.peer_id);
		auto snode = ring.succ(msg.msg.join.peer_id);
		cout << "\tsucc is "
			<< hash_to_string<HASHSZ>(snode->hash) << "\n";
		char* pred0 = snode->prevs[0]->prevs[0]->hash;
		char* pred1 = snode->prevs[0]->hash;
		char* succ0 = snode->nexts[0]->hash;
		char* succ1 = snode->nexts[0]->nexts[0]->hash;

		storage_msg send_msg;
		send_msg.type = STORAGE_MSG_INIT;
		memcpy(send_msg.msg.init.pred0, pred0, HASHSZ);
		memcpy(send_msg.msg.init.pred1, pred1, HASHSZ);
		memcpy(send_msg.msg.init.succ,  succ0, HASHSZ);

		send_msg_to_storage(fd, msg.msg.join.peer_id, send_msg);

		if (snode->prevs[0] != snode) {
			cout << "\tsending join to pred1\n";
			send_msg_to_storage(fd, pred1, msg);

			if (snode->nexts[0] != snode->prevs[0]) {
				cout << "\tsending join to succ\n";
				send_msg_to_storage(fd, succ0, msg);

				if (snode->nexts[0]->nexts[0] != snode->prevs[0]) {
					cout << "\tsending join to succ\n";
					send_msg_to_storage(fd, succ1, msg);
				}
			}
		}
	}
	cout << "done handling msg event\n";
}

void
GTStoreManager::mainloop() {
	
	cout << "Inside GTStoreManager::mainloop()\n";


	while (true) {
		handle_msg(recv_msg_fd);
	}
}

int main() {
	GTStoreManager manager;
	manager.init();
	manager.mainloop();
}
