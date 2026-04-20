TARGET 		:= udp
EXAMPLES    := $(shell ls -d ./examples/*/)
CC 		  	:= g++ -std=c++11
LDFLAGS		:= 
IFLAGS		:= -I$(CURDIR)/include
BUILD		:= build

.PHONY: clean, examples, lib

examples:
	@for dir in $(EXAMPLES); do \
		echo "Building $$dir"; \
		$(MAKE) -C $$dir; \
	done
	

lib:
	$(MAKE) -C lib

clean:
	@for dir in $(EXAMPLES); do \
		echo "Building $$dir"; \
		$(MAKE) clean -C $$dir; \
	done
	rm -rf lib/x86 lib/x64