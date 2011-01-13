CC=gcc
CFLAGS=-DUNITTEST -fexceptions -fstack-protector-all -pthread -O2 -g -Wall -Wextra -Wno-unknown-pragmas -Werror-implicit-function-declaration -Wno-unused-parameter -Wdeclaration-after-statement -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes  -I$(shell pwd)/include -I$(shell pwd)/../sockaddrutil/include


LDFLAGS= -lssl -lcheck 
SOURCES=src/stunlib.c src/stun_os.c src/stunclient.c src/turnclient.c test/testmain.c test/stunlib_test.c test/stunclient_test.c ../sockaddrutil/src/sockaddr_util.c 
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=testmain

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS)$< -o $@

clean:
	rm -rf src/*o test/*o $(EXECUTABLE)
