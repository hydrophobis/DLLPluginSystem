cd plugins\console
clang++.exe -shared -o console.dll .\console.cc

cd ../heartbeat
clang++.exe -shared -o heartbeat.dll .\heartbeat.cc

cd ../logger
clang++.exe -shared -o logger.dll .\logger.cc

cd ../manager
clang++.exe -shared -o manager.dll .\manager.cc

cd ..\..
clang++.exe -o runtime .\runtime.cc .\ini.cc