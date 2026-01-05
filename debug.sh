set -e
./build.sh
wine64 ./misc/raddbg.exe --bin --rdi build/main.pdb
rm -rf ./build/main.pdb
wine64 misc/raddbg.exe

