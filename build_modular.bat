@echo off
echo Building ServerControl (Fully Modular for Windows)...

if not exist files mkdir files

echo Building modular server...
cd server
g++ -std=c++17 -DASIO_STANDALONE -D_WEBSOCKETPP_CPP11_THREAD_ -D_WIN32_WINNT=0x0601 -I../include ^
    main_modular.cpp ^
    platform/PlatformAbstraction.cpp ^
    websocket/WebSocketManager.cpp ^
    stats/StatsMonitor.cpp ^
    tasks/TaskExecutor.cpp ^
    -o ../files/servercontrol_modular.exe -lws2_32 -liphlpapi -lpsapi
cd ..

echo.
echo Build complete!
echo   Modular Server binary: .\files\servercontrol_modular.exe
echo.
echo To run the server with IP assignment, use:
echo   Run as Administrator: .\files\servercontrol_modular.exe
echo.
dir files\servercontrol_modular.exe
