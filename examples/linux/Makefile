EXAMPLES    := examples
CC 		  	:= g++ -std=c++11
LDFLAGS		:= 
IFLAGS		:= -I$(CURDIR)/../../include
BUILD		:= build

.PHONY: clean, with-wsl

with-wsl:
	wsl make client -f Makefile
	wsl make server -f Makefile

server: source/server.cpp
	mkdir -p $(BUILD)
	$(CC) -g $(CURDIR)/source/server.cpp $(IFLAGS) $(LDFLAGS) $(LDLIBS) -o $(CURDIR)/$(BUILD)/server

client: source/client.cpp
	mkdir -p $(BUILD)
	$(CC) -g $(CURDIR)/source/client.cpp $(IFLAGS) $(LDFLAGS) $(LDLIBS) -o $(CURDIR)/$(BUILD)/client

clean:
	rm -rf $(BUILD)