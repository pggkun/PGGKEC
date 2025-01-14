TARGET 		:= udp_windows
CC 		  	:= g++ -std=c++11
LDFLAGS		:= C:/MinGW/lib/libpthread.a -lws2_32
IFLAGS		:= -I$(CURDIR)/../../include -IC:/MinGW/include
BUILD 		:= build

.PHONY: clean

server: source/server.cpp
	mkdir -p $(BUILD)
	$(CC) -g $(CURDIR)/source/server.cpp $(IFLAGS) $(LDFLAGS) $(LDLIBS) -o $(CURDIR)/$(BUILD)/server

client: source/client.cpp
	mkdir -p $(BUILD)
	$(CC) -g $(CURDIR)/source/client.cpp $(IFLAGS) $(LDFLAGS) $(LDLIBS) -o $(CURDIR)/$(BUILD)/client

clean:
	rm -rf build