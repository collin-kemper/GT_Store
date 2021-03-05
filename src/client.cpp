#include "gtstore.hpp"
#include "hash/blake2.h"

#define HASHSZ 16

void GTStoreClient::init(int id) {
	client_id = id;
	blake2b(hash_id, sizeof(hash_id), &id, sizeof(id), 0, 0);
	string name = hash_to_client_name<HASHSZ>(hash_id);

	recv_msg_fd = recv_msg_socket(name);
}

val_t GTStoreClient::get(string key) {
	
	val_t value;

	// Get the value!
	char hashkey[HASHSZ];
	blake2b(hashkey, sizeof(hashkey), key.c_str(), key.size(), 0, 0);

	cout << "getting key " << key << " with hash "
		<< hash_to_string<HASHSZ>(hashkey) << "\n";

	storage_msg msg;
	msg.type = STORAGE_MSG_GET;
	memcpy(msg.msg.get.hash, hashkey, HASHSZ);
	memcpy(msg.msg.get.cli_id, hash_id, HASHSZ);

	send_get(recv_msg_fd, MANAGER_MSG, hashkey, hash_id);
	char outkey[HASHSZ];
	recv_put(recv_msg_fd, outkey, value);

	return value;
}

bool GTStoreClient::put(string key, val_t value) {

	string print_value = "";
	for (uint i = 0; i < value.size(); i++) {
		print_value += value[i] + " ";
	}

	char hashkey[HASHSZ];
	blake2b(hashkey, sizeof(hashkey), key.c_str(), key.size(), 0, 0);

	cout << "putting key " << key << " with hash "
		<< hash_to_string<HASHSZ>(hashkey) << "\n";

	char val_arr[1024];
	bzero(val_arr, 1024);
	char* vap = (char*) val_arr;
	for (int i = 0; i < (int) value.size(); ++i) {
		memcpy(vap, value[i].c_str(), value[i].size() + 1);
		vap += value[i].size() + 1;
	}

	send_put(recv_msg_fd, MANAGER_MSG, hashkey, val_arr, vap - val_arr);

	return true;
}

void GTStoreClient::finalize() {
}
