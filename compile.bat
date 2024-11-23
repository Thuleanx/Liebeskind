cmake -S . -B .\build\ -G "MinGW Makefiles" -DBUILD_SHARED_LIBS=off -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -D CMAKE_CXX_COMPILER=g++
(cd ./build/ && make)
