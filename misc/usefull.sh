x86_64-w64-mingw32-objdump -p src/win32_handmade.exe | less
clangd --compile-commands-dir=build --check=src/win32_handmade.cpp --check-locations=false

