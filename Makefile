TARGET 		:= udp
EXAMPLES    := examples
CC 		  	:= g++ -std=c++11
LDFLAGS		:= 
IFLAGS		:= -I$(CURDIR)/include
BUILD		:= build

.PHONY: clean

clean:
	@find $(EXAMPLES) -type d -name $(BUILD) -exec rm -rf {} +