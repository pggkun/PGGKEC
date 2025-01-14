source ~/.bashrc
export VITASDK=/usr/local/vitasdk
export PATH=$VITASDK/bin:$PATH
mkdir -p build
cd build
cmake ..
make