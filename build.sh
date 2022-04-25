git clone https://github.com/monero-ecosystem/monero-cpp.git
cd monero-cpp/external/monero-project
make release-static && make release-static
cd ../../../
mkdir build
cd build
cmake ..
cmake --build .
