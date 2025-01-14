source ~/.bashrc
export PSPDEV=/usr/local/pspdev
export PATH=$PSPDEV/bin:$PATH
mkdir -p build
make -f Makefile.wsl
mv *.PBP build/
mv *.SFO build/
mv *.elf build/
rm -f source/*.o