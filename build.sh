(
  cd build || exit 1
  bear --append -- $CXX ../src/main.cpp -g3 -gcodeview -o main.exe -luser32 -lgdi32
  time "$CXX" \
    ../src/main.cpp \
    -g3 \
    -gcodeview \
    -o main.exe \
    -l user32 \
    -l gdi32
)

