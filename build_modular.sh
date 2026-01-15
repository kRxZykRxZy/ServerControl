#!/bin/bash

set -e

echo "Building ServerControl (Fully Modular)..."

# Create files directory if it doesn't exist
mkdir -p files

# Build modular server
echo "Building modular server..."
cd server
g++ -std=c++17 -pthread -DASIO_STANDALONE -D_WEBSOCKETPP_CPP11_THREAD_ -I../include \
    main_modular.cpp \
    platform/PlatformAbstraction.cpp \
    websocket/WebSocketManager.cpp \
    stats/StatsMonitor.cpp \
    tasks/TaskExecutor.cpp \
    -o ../files/servercontrol_modular 2>&1 | grep -v "warning:" || true
cd ..

# Build control
echo "Building control..."
cd control
g++ -std=c++17 -pthread -I../include \
    main.cpp \
    ui/UI.cpp \
    core/TaskManager.cpp \
    config/Config.cpp \
    network/Server.cpp \
    http/HttpClient.cpp \
    -o ../files/control -lncurses 2>&1 | grep -v "warning:" || true
cd ..

echo "Build complete!"
echo "  Modular Server binary: ./files/servercontrol_modular"
echo "  Control binary: ./files/control"
ls -lh files/servercontrol_modular files/control 2>/dev/null || true
