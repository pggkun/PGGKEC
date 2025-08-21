.PHONY: clean

all:
	rm -rf x64 x86
	wsl make x64 -f Makefile.wsl
	wsl make x86 -f Makefile.wsl

x64:
	wsl make x64 -f Makefile.wsl

x86:
	wsl make x86 -f Makefile.wsl

clean:
	rm -rf x64 x86
