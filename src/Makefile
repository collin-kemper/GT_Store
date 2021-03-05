CFLAGS  = -O3 -Wall -Wextra -g
LFLAGS  =
CC      = g++ -std=c++11
RM      = /bin/rm -rf

TESTS = test_app manager storage multiadd
LIBS = gtstore.cpp hash/obj/blake2b.o

all: $(TESTS)

manager: manager.cpp $(LIBS)
	$(CC) $(CFLAGS) $^ -o bin/$@

storage: storage.cpp $(LIBS)
	$(CC) $(CFLAGS) $^ -o bin/$@

test_app : test_app.cpp client.cpp $(LIBS)
	$(CC) $(CFLAGS) $^ -o bin/$@

multiadd: multiadd.cpp client.cpp $(LIBS)
	$(CC) $(CFLAGS) $^ -o bin/$@

hash/obj/blake2b.o: hash/*
	make -C hash

clean:
	$(RM) bin/* hash/obj/*.o sockets/*
