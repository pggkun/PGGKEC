TARGET 		:= udp_windows
CC 		  	:= gcc -DVERBOSE
LDFLAGS		:= C:/MinGW/lib/libpthread.a -lws2_32
IFLAGS		:= -I$(CURDIR)/../../include -IC:/MinGW/include
BUILD 		:= build

.PHONY: clean

server: source/server.c
	mkdir -p $(BUILD)
	$(CC) -g $(CURDIR)/source/server.c $(IFLAGS) $(LDFLAGS) $(LDLIBS) -o $(CURDIR)/$(BUILD)/server

client: source/client.c
	mkdir -p $(BUILD)
	$(CC) -g $(CURDIR)/source/client.c $(IFLAGS) $(LDFLAGS) $(LDLIBS) -o $(CURDIR)/$(BUILD)/client

clean:
	rm -rf build