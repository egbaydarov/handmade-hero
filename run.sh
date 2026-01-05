set -e
./build.sh -O3 -DNDEBUG
wine64 build/main.exe 

