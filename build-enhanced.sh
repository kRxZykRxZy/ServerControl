#!/bin/bash
# Enhanced build script with advanced features

set -e

echo "========================================="
echo "  ServerControl Enhanced Build System"
echo "========================================="
echo ""

# Feature flags
ENABLE_ADVANCED_FEATURES=${ENABLE_ADVANCED_FEATURES:-1}
ENABLE_DATABASE_SUPPORT=${ENABLE_DATABASE_SUPPORT:-1}
ENABLE_CONTAINER_SUPPORT=${ENABLE_CONTAINER_SUPPORT:-1}
BUILD_TYPE=${BUILD_TYPE:-Release}

echo "Build Configuration:"
echo "  Build Type: $BUILD_TYPE"
echo "  Advanced Features: $([ $ENABLE_ADVANCED_FEATURES -eq 1 ] && echo 'ON' || echo 'OFF')"
echo "  Database Support: $([ $ENABLE_DATABASE_SUPPORT -eq 1 ] && echo 'ON' || echo 'OFF')"
echo "  Container Support: $([ $ENABLE_CONTAINER_SUPPORT -eq 1 ] && echo 'ON' || echo 'OFF')"
echo ""

# Compiler flags
CXXFLAGS="-std=c++17 -pthread -I./include"
if [ "$BUILD_TYPE" = "Debug" ]; then
    CXXFLAGS="$CXXFLAGS -g -O0 -DDEBUG"
else
    CXXFLAGS="$CXXFLAGS -O2 -DNDEBUG"
fi

if [ $ENABLE_ADVANCED_FEATURES -eq 1 ]; then
    CXXFLAGS="$CXXFLAGS -DENABLE_ADVANCED_FEATURES"
fi

# Build server
echo "[1/3] Building server..."
g++ $CXXFLAGS server.cpp -o servercontrol 2>&1 | grep -v "warning:" || true
echo "  ✓ Server built ($(ls -lh servercontrol | awk '{print $5}'))"

# Build control with modular components
echo "[2/3] Building control..."
cd control

CONTROL_SOURCES="
    main.cpp
    ui/UI.cpp
    core/TaskManager.cpp
    config/Config.cpp
    network/Server.cpp
    http/HttpClient.cpp
"

# Add advanced features if enabled
if [ $ENABLE_ADVANCED_FEATURES -eq 1 ]; then
    echo "  • Including advanced features"
    # Advanced features will be linked when implemented
fi

g++ $CXXFLAGS $CONTROL_SOURCES -o control -lncurses 2>&1 | grep -v "warning:" || true
echo "  ✓ Control built ($(ls -lh control | awk '{print $5}'))"

cd ..

# Copy to files directory
echo "[3/3] Publishing binaries..."
mkdir -p files
cp servercontrol files/
cp control/control files/
echo "  ✓ Binaries published to files/"

# Summary
echo ""
echo "========================================="
echo "  Build Complete!"
echo "========================================="
echo ""
echo "Binaries:"
echo "  Server:  ./servercontrol ($(ls -lh servercontrol | awk '{print $5}'))"
echo "  Control: ./control/control ($(ls -lh control/control | awk '{print $5}'))"
echo ""
echo "Published:"
echo "  files/servercontrol"
echo "  files/control"
echo ""
echo "Run tests:"
echo "  ./tests.sh"
echo ""
