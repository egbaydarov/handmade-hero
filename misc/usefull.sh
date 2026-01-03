x86_64-w64-mingw32-objdump -p src/main.exe | less
clangd --compile-commands-dir=build --check=src/main.cpp --check-locations=false

