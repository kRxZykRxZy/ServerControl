#!/bin/bash

set -e

echo "Building ServerControl (Modular)..."

# Build server with modular components
echo "Building server..."
cd server
g++ -std=c++17 -pthread -I../include \
    -c core/Utils.cpp -o core/Utils.o
g++ -std=c++17 -pthread -I../include \
    -c tasks/Task.cpp -o tasks/Task.o
g++ -std=c++17 -pthread -I../include \
    -c tasks/TaskManager.cpp -o tasks/TaskManager.o
g++ -std=c++17 -pthread -I../include \
    -c monitoring/Stats.cpp -o monitoring/Stats.o
g++ -std=c++17 -pthread -I../include \
    -c network/WebSocket.cpp -o network/WebSocket.o
g++ -std=c++17 -pthread -I../include \
    -c network/Discovery.cpp -o network/Discovery.o
g++ -std=c++17 -pthread -I../include \
    -c network/NetworkConfig.cpp -o network/NetworkConfig.o
g++ -std=c++17 -pthread -I../include \
    -c system/RemoteDesktop.cpp -o system/RemoteDesktop.o -lX11 -lXtst

# Link server (still using monolithic server.cpp for now, will modularize fully later)
cd ..
g++ -std=c++17 -pthread -I./include \
    server.cpp \
    server/core/Utils.o \
    server/tasks/Task.o \
    server/tasks/TaskManager.o \
    server/monitoring/Stats.o \
    server/network/WebSocket.o \
    server/network/Discovery.o \
    server/network/NetworkConfig.o \
    server/system/RemoteDesktop.o \
    -o servercontrol -lX11 -lXtst

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
echo "  Server binary: ./servercontrol"
echo "  Control binary: ./control/control"
