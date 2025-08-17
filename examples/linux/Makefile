EXAMPLES    := examples
CC 		  	:= gcc -DVERBOSE
LDFLAGS		:= 
IFLAGS		:= -I$(CURDIR)/../../include
BUILD		:= build

.PHONY: clean, with-wsl

with-wsl:
	wsl make client -f Makefile
	wsl make server -f Makefile

server: source/server.c
	mkdir -p $(BUILD)
	$(CC) -g $(CURDIR)/source/server.c $(IFLAGS) $(LDFLAGS) $(LDLIBS) -o $(CURDIR)/$(BUILD)/server

client: source/client.c
	mkdir -p $(BUILD)
	$(CC) -g $(CURDIR)/source/client.c $(IFLAGS) $(LDFLAGS) $(LDLIBS) -o $(CURDIR)/$(BUILD)/client

clean:
	rm -rf $(BUILD)