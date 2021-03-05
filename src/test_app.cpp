#include "gtstore.hpp"

void
show_help()
{
	cout << "valid commands are:\n\
	get \"<key>\"\n\
	put \"<key>\" \"<val0>\" \"<val1>\" <...> \"<valn>\"\n";
}

int
parse_string(
	string& in,
	int offset,
	string& out)
{
	bool in_string = false;
	bool escaped = false;
	while (true) {
		if (offset >= (int) in.size()) {
			if (in_string) {
				return -1;
			}
			return 0;
		}
		if (in_string) {
			if (escaped) {
				escaped = false;
				out.push_back(in[offset]);
			}
			else if (in[offset] == '\\') {
				escaped = true;
			}
			else if (in[offset] == '\"') {
				return offset+1;
			}
			else {
				out.push_back(in[offset]);
			}
		}
		else if (in[offset] == '\"') {
			in_string = true;
		}
		offset += 1;
	}
}

void
print_value(
	vector<string>& value)
{
	cout << "\tvalue:\n";
	for (auto v : value) {
		cout << "\t\t\"" << v << "\"\n";
	}
}

void
get_cmd(
	GTStoreClient& client,
	string& cmd)
{

	string key;
	int offset = 0;
	offset = parse_string(cmd, offset, key);
	if (offset == 0) {
		cout << "error parsing command: could not find key\n";
		show_help();
	}
	else if (parse_string(cmd, offset, key) != 0) {
		cout << "error parsing command: string after key\n";
		show_help();
	}
	else {
		cout << "\tkey: " << key << "\n";
	}

	val_t value = client.get(key);
	print_value(value);
}

void
put_cmd(
	GTStoreClient& client,
	string& cmd)
{
	string key;
	vector<string> value;
	int offset = 0;
	offset = parse_string(cmd, offset, key);
	if (offset == 0) {
		cout << "error parsing command: could not find key\n";
		show_help();
		return;
	}
	do {
		value.push_back("");
		offset = parse_string(cmd, offset, value.back());
	} while (offset != 0);
	value.pop_back();

	client.put(key, value);
}

int main() {
	pid_t pid = getpid();
	GTStoreClient client;
	client.init(pid);
	
	string cmd;
	while (true) {
		cout << "command: ";
		getline(cin, cmd);
		if (cmd.size() < 4) {
			show_help();
			continue;
		}
		if (cmd.compare(0, 4, "get ") == 0) {
			get_cmd(client, cmd);
		}
		else if (cmd.compare(0, 4, "put ") == 0) {
			put_cmd(client, cmd);
		}
		else {
			show_help();
			continue;
		}
	}

	client.finalize();
}
