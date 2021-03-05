#include "gtstore.hpp"

/**************** helper functions ****************/
void
error(
	std::string msg)
{
	perror(msg.c_str());
	exit(-1);
}

int
recv_msg_socket(
	string recv_name)
{
	int recvfd;
	struct sockaddr_un serv_addr;

	recvfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (recvfd < 0)
		error("server error failed to open socket");
	serv_addr.sun_family = AF_UNIX;
	memcpy(serv_addr.sun_path, recv_name.c_str(), recv_name.size() + 1);
	if (bind(recvfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("server error binding");
	
	return recvfd;
}

void
recv_and_print(
  int fd)
{
  char buf[256];
  bzero(buf, 256);
  int len = recv(fd, buf, sizeof(buf), 0);
  printf("recieved: %d chars %s\n", len, buf);
}

void
add_sock_epoll(
  int epfd,
  int sockfd)
{
  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = sockfd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev) < 0)
    error("error in epoll_ctl");
}

void
read_buf_to_vec(
  vector<string>& out,
  char* buf,
  size_t buf_len)
{
	out.push_back("");
	size_t word_len = 0;
  for (int i = 0; i < (int) buf_len; ++i) {
    if (buf[i] == '\0') {
			if (word_len != 0)
				out.push_back("");
      word_len = 0;
      continue;
    }
    out.back().push_back(buf[i]);
    word_len += 1;
  }
  out.pop_back();
}

void
send_put(
	int fd,
	const string& name,
  char key[HASHSZ],
  char* val,
  size_t val_sz)
{
  storage_msg msg;
  bzero(&msg.msg.put.val, 1024);
  msg.type = STORAGE_MSG_PUT;
  memcpy(&msg.msg.put.key, key, HASHSZ);
  memcpy(&msg.msg.put.val, val, val_sz);

  struct sockaddr_un dest_addr;
  dest_addr.sun_family = AF_UNIX;
  memcpy(dest_addr.sun_path, name.c_str(), name.size() + 1);
  sendto(fd, (char*) &msg, sizeof(msg), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
}

void
send_init(
  int fd,
	const string& name,
  char pred0[HASHSZ],
  char pred1[HASHSZ],
  char succ[HASHSZ])
{
  storage_msg msg;
  msg.type = STORAGE_MSG_INIT;
  memcpy(&msg.msg.init.pred0, pred0, HASHSZ);
  memcpy(&msg.msg.init.pred1, pred1, HASHSZ);
  memcpy(&msg.msg.init.succ,  succ,  HASHSZ);

  send(fd, (char*) &msg, sizeof(msg), 0);

  struct sockaddr_un dest_addr;
  dest_addr.sun_family = AF_UNIX;
  memcpy(dest_addr.sun_path, name.c_str(), name.size() + 1);
  sendto(fd, (char*) &msg, sizeof(msg), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
}

void
send_join (
  int fd,
	const string& name,
  char peer_id[HASHSZ])
{
  storage_msg msg;
  msg.type = STORAGE_MSG_JOIN;
  memcpy(&msg.msg.join.peer_id, peer_id, HASHSZ);

  struct sockaddr_un dest_addr;
  dest_addr.sun_family = AF_UNIX;
  memcpy(dest_addr.sun_path, name.c_str(), name.size() + 1);
  sendto(fd, (char*) &msg, sizeof(msg), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
}

void
send_get (
	int fd,
	const string& name,
  char hash[HASHSZ],
  char cli_id[HASHSZ])
{
  storage_msg msg;
  msg.type = STORAGE_MSG_GET;
  memcpy(&msg.msg.get.hash, hash, HASHSZ);
  memcpy(&msg.msg.get.cli_id, cli_id, HASHSZ);

  struct sockaddr_un dest_addr;
  dest_addr.sun_family = AF_UNIX;
  memcpy(dest_addr.sun_path, name.c_str(), name.size() + 1);
  sendto(fd, (char*) &msg, sizeof(msg), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
}

void
print_msg(
  char dst_id[HASHSZ],
  storage_msg& msg)
{
  cout << "\tsending message to " << hash_to_string<HASHSZ>(dst_id) << "\n";
  if (msg.type == STORAGE_MSG_PUT) {
    cout << "\t\tput:\n";
    cout << "\t\tkey: " << hash_to_string<HASHSZ>(msg.msg.put.key) << "\n";
  }
  else if (msg.type == STORAGE_MSG_JOIN) {
    cout << "\t\tjoin:\n";
    cout << "\t\tpeer_id: " << hash_to_string<HASHSZ>(msg.msg.join.peer_id) << "\n";
  }
  else if (msg.type == STORAGE_MSG_GET) {
    cout << "\t\tget:\n";
    cout << "\t\thash:   " << hash_to_string<HASHSZ>(msg.msg.get.hash) << "\n";
    cout << "\t\tcli_id: " << hash_to_string<HASHSZ>(msg.msg.get.cli_id) << "\n";
  }
  else if (msg.type == STORAGE_MSG_INIT) {
    cout << "\t\tinit:\n";
    cout << "\t\tpred0: " << hash_to_string<HASHSZ>(msg.msg.init.pred0) <<  "\n";
    cout << "\t\tpred1: " << hash_to_string<HASHSZ>(msg.msg.init.pred1) <<  "\n";
    cout << "\t\tsucc:  " << hash_to_string<HASHSZ>(msg.msg.init.succ) <<  "\n";
  }
  else {
    cout << "\t\t???\n";
  }
}

void
send_msg_to_storage(
	int fd,
	char hash[HASHSZ],
	storage_msg& msg)
{
	print_msg(hash, msg);
	string name = hash_to_storage_name<HASHSZ>(hash);

  struct sockaddr_un dest_addr;
  dest_addr.sun_family = AF_UNIX;
  memcpy(dest_addr.sun_path, name.c_str(), name.size() + 1);
  sendto(fd, (char*) &msg, sizeof(msg), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
}

void
recv_put(
	int fd,
  char key[HASHSZ],
  vector<string>& val)
{
  storage_msg msg;

  recv(fd, (char*) &msg, sizeof(msg), 0);
  memcpy(key, msg.msg.put.key, HASHSZ);
  read_buf_to_vec(val, msg.msg.put.val, 1024);
}
