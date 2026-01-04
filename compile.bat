clang++.exe -shared -o example.dll .\example.cc -Wl
mv example.dll plugins\example.dll

cd plugins/echo
clang++.exe -shared -o echo.dll .\echo.cc -Wl

cd ../config
clang++.exe -shared -o config.dll .\config.cc -Wl

cd ../heartbeat
clang++.exe -shared -o heartbeat.dll .\heartbeat.cc -Wl

cd ../logger
clang++.exe -shared -o logger.dll .\logger.cc -Wl

cd ..\..
clang++.exe -o runtime .\runtime.cc .\ini.cc