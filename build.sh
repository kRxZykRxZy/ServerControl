#!/bin/bash

set -e

echo "Building ServerControl..."

# Build server
echo "Building server..."
g++ -std=c++17 -pthread server.cpp -o server

# Build control
echo "Building control..."
cd control
g++ -std=c++17 -pthread main.cpp UI.cpp TaskManager.cpp Config.cpp Server.cpp HttpClient.cpp -o control -lncurses
cd ..

echo "Build complete!"
echo "  Server binary: ./server"
echo "  Control binary: ./control/control"
