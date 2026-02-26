# configure
cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_CXX_FLAGS="-march=native" -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -S. -B./build
# build
cmake --build ./build --config Release --target all -j 4 --
