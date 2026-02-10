set -e
./build.sh -O3 -DDEBUG
wine64 build/win32_handmade.exe 

