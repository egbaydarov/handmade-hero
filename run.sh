set -e
./build.sh -O5 -DNDEBUG
wine64 build/main.exe 

