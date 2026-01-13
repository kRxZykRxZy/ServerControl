#!/bin/bash

set -e

echo "Building ServerControl..."

# Build server (monolithic for now, will modularize step by step)
echo "Building server..."
g++ -std=c++17 -pthread -I./include server.cpp -o servercontrol 2>&1 | grep -v "warning:"

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
    -o control -lncurses 2>&1 | grep -v "warning:"
cd ..

echo "Build complete!"
echo "  Server binary: ./servercontrol"
echo "  Control binary: ./control/control"
