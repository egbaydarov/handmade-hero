set -e
./build.sh -O3 -DNDEBUG
wine64 build/win32_handmade.exe 

