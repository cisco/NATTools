CC=gcc
CFLAGS=-fexceptions -fstack-protector-all -pthread -O2 -g -Wall -Wextra -Wno-unknown-pragmas -Werror-implicit-function-declaration -Werror -Wno-unused-parameter -Wdeclaration-after-statement -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes
LDFLAGS=-lcheck -Iinclude
SOURCES=test/testmain.c test/sockaddr_test.c test/icelib_test.c src/icelib.c src/sockaddr_util.c src/icelibtypes.c
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=testmain

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o $(EXECUTABLE)
