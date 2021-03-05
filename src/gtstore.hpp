#ifndef GTSTORE
#define GTSTORE

#include <cstdlib>
#include <unistd.h>

#include <string>
#include <vector>

#include <cstdio>
#include <iostream>

#include <sys/wait.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <sys/epoll.h>

#include "hashtable.hpp"

#define MANAGER_MSG "sockets/manager-msg"
#define MANAGER_STREAM "sockets/manager-stream"
#define HASHSZ 16

using namespace std;

typedef vector<string> val_t;

class GTStoreClient {
private:
public:
	int client_id;
	char hash_id[HASHSZ];

	int recv_msg_fd;
	int send_msg_fd;

	/* functions */
	void init(int id);
	void finalize();
	val_t get(string key);
	bool put(string key, val_t value);
};

class GTStoreManager {
private:
public:
	int recv_msg_fd;

	skiplist<HASHSZ, 5> ring;

	/* functions */
	void init();
	void mainloop();

	void handle_msg(int fd);
};

class GTStoreStorage {
public:
	char hash_id[HASHSZ];

	int recv_msg_fd;

	hashtable_t<HASHSZ> htable;

	char pred0[HASHSZ];
	char pred1[HASHSZ];
	char succ[HASHSZ];

	GTStoreStorage();

	void init();

	void send_data_range(char dst_id[HASHSZ], char begin[HASHSZ],
		char end[HASHSZ]);

	void handle_join(char peer_id[HASHSZ]);
	void handle_get(char hash[HASHSZ], char cli_id[HASHSZ]);
	void handle_msg(int fd);

	void handle_event(int epfd, struct epoll_event& event);
	void mainloop();
};



enum msg_type {
	STORAGE_MSG_JOIN,
	STORAGE_MSG_GET,
	STORAGE_MSG_INIT,

	STORAGE_MSG_PUT,
};
struct storage_msg {
	msg_type type;
	union {
		struct {
			char hash[HASHSZ];
			char cli_id[HASHSZ];
		} get;
		struct {
			char peer_id[HASHSZ];
		} join;
		struct {
			char pred0[HASHSZ];
			char pred1[HASHSZ];
			char succ[HASHSZ];
		} init;
		struct {
			char key[HASHSZ];
			char val[1024];
		} put;
	} msg;
};

/**************** helper functions ****************/
void error(std::string msg);
int recv_msg_socket(string recv_name);
int send_msg_socket(string recv_name);

void add_sock_epoll(int epfd, int sockfd);

void send_get(int fd, const string& name, char hash[HASHSZ], char cli_id[HASHSZ]);
void send_join(int fd, const string& name, char peer_id[HASHSZ]);
void send_init(int fd, const string& name, char pred0[HASHSZ],
	char pred1[HASHSZ], char succ[HASHSZ]);
void send_put(int fd, const string& name, char key[HASHSZ], char* val, size_t val_sz);

void send_msg_to_storage(int fd, char hash[HASHSZ], storage_msg& msg);

void recv_put(int fd, char key[HASHSZ], vector<string>& val);


/**************** impl functions ****************/
template<size_t H>
string
hash_to_string(
	char hash[H])
{
	char characters[] =
	"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@";
	string ret = "";
	for (int i = 0; i < (int) H/2; ++i) {
		unsigned char h0 = (unsigned char) hash[i];
		unsigned char h1 = (unsigned char) hash[i + 1];
		unsigned group = (((unsigned)h1)<<8) | ((unsigned)h0);
		ret.push_back(characters[(group)&0x3F]);
		ret.push_back(characters[(group>>6)&0x1F]);
		ret.push_back(characters[(group>>11)&0x1F]);
	}
	if (H%2 == 1) {
		ret.push_back(characters[hash[H-1]&0x3F]);
		ret.push_back(characters[hash[H-1]>>6&0x3F]);
	}

	return ret;
}
template<size_t H>
string
hash_to_client_name(
	char hash[H])
{
	return "sockets/client-" + hash_to_string<H>(hash);
}
template<size_t H>
string
hash_to_storage_name(
	char hash[H])
{
	return "sockets/storage-" + hash_to_string<H>(hash);
}
#endif
