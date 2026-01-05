set -e
./build.sh
wine64 ./misc/raddbg.exe --bin --rdi build/win32_handmade.pdb
rm -rf ./build/win32_handmade.pdb
wine64 misc/raddbg.exe

