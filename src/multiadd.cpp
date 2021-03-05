#include "gtstore.hpp"

#include <stdlib.h>

int main(
	int argv,
	char* argc[]) {

	if (argv != 2) {
		cout << "Please provide a single argument for number of key values pairs to put\n";
	}

	pid_t pid = getpid();
	GTStoreClient client;
	client.init(pid);

	int count = atoi(argc[1]);
	for (int i = 0; i < count; ++i) {
		string tmp = to_string(i);
		client.put(tmp, {tmp});
	}
	
	client.finalize();
}
