#!/bin/bash

set -e

echo "Building ServerControl (Modular)..."

# Create files directory if it doesn't exist
mkdir -p files

# Build server (modular - all code in server/main.cpp)
echo "Building server from server/main.cpp..."
cd server
g++ -std=c++17 -pthread -I../include \
    main.cpp \
    -o ../files/servercontrol 2>&1 | grep -v "warning:" || true
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
echo "  Server binary: ./files/servercontrol"
echo "  Control binary: ./files/control"
ls -lh files/servercontrol files/control 2>/dev/null || true
