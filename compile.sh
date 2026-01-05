cd plugins

echo --- CONSOLE ---
clang++ -fPIC -c console.cc -o console.o
clang++ -shared -o console.so console.o

echo --- PYTHON ---
clang++ -fPIC -c python.cc -I"/usr/include/python3.12" -o python.o
clang++ -fPIC -c ../ini.cc -o ini.o
clang++ -shared -o python.so python.o ini.o -L"/lib/x86_64-linux-gnu/" -lpython3.12

cd ..
echo --- RUNTIME ---
clang++ -o runtime runtime.cc ini.cc