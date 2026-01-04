cd plugins
clang++.exe -shared -o console.dll .\console.cc
clang++.exe -shared -o heartbeat.dll .\heartbeat.cc
clang++.exe -shared -o logger.dll .\logger.cc
clang++.exe -shared -o python.dll .\python.cc ..\ini.cc -I"C:\Users\Kasen\AppData\Local\Programs\Python\Python314\include" -L"C:\Users\Kasen\AppData\Local\Programs\Python\Python314\libs" -lpython314

cd ..
clang++.exe -o runtime .\runtime.cc .\ini.cc