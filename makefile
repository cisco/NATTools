CC=gcc
CFLAGS=-fexceptions -fstack-protector-all -pthread -O2 -g -Wall -Wextra -Wno-unknown-pragmas -Werror-implicit-function-declaration -Werror -Wno-unused-parameter -Wdeclaration-after-statement -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -I$(shell pwd)/include -I$(shell pwd)/../stunlib/include -I$(shell pwd)/../sockaddrutil/include
LDFLAGS=-lssl
SOURCES= src/main.c ../sockaddrutil/src/sockaddr_util.c ../stunlib/src/stunlib.c ../stunlib/src/stun_os.c ../stunlib/src/stunclient.c ../stunlib/src/turnclient.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=ice

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf src/o* *o $(EXECUTABLE)
