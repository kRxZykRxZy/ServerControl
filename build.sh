#!/bin/bash

set -e

echo "Building ServerControl..."

# Build server
echo "Building server..."
g++ -std=c++17 -pthread -I./include server.cpp -o server

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
    -o control -lncurses
cd ..

echo "Build complete!"
echo "  Server binary: ./server"
echo "  Control binary: ./control/control"
