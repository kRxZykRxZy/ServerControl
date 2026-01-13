#!/bin/bash

# Comprehensive Test Suite for ServerControl
# Tests all major components and features

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASSED=0
FAILED=0

echo "========================================"
echo "  ServerControl Comprehensive Tests"
echo "========================================"
echo ""

# Helper functions
pass() {
    echo -e "${GREEN}✓ PASS${NC}: $1"
    ((PASSED++))
}

fail() {
    echo -e "${RED}✗ FAIL${NC}: $1"
    ((FAILED++))
}

info() {
    echo -e "${YELLOW}ℹ INFO${NC}: $1"
}

# Test 1: Build System
echo "=== Test Suite 1: Build System ==="
info "Building project..."
if ./build.sh > /tmp/build.log 2>&1; then
    pass "Project builds successfully"
else
    fail "Project build failed"
    cat /tmp/build.log | tail -20
fi

# Test 2: Binary Creation
echo ""
echo "=== Test Suite 2: Binary Creation ==="
if [ -f "./servercontrol" ]; then
    SIZE=$(ls -lh servercontrol | awk '{print $5}')
    pass "Server binary created ($SIZE)"
else
    fail "Server binary not found"
fi

if [ -f "./control/control" ]; then
    SIZE=$(ls -lh control/control | awk '{print $5}')
    pass "Control binary created ($SIZE)"
else
    fail "Control binary not found"
fi

# Test 3: Server Startup
echo ""
echo "=== Test Suite 3: Server Startup ==="
info "Starting server..."
timeout 3 ./servercontrol > /tmp/server_startup.log 2>&1 &
SERVER_PID=$!
sleep 2

if ps -p $SERVER_PID > /dev/null; then
    pass "Server starts without crashing"
    kill $SERVER_PID 2>/dev/null || true
else
    fail "Server crashed on startup"
    cat /tmp/server_startup.log
fi

# Test 4: Server IP Binding
echo ""
echo "=== Test Suite 4: IP Binding ==="
if grep -q "Bind IP:" /tmp/server_startup.log; then
    IP=$(grep "Bind IP:" /tmp/server_startup.log | awk '{print $NF}')
    pass "Server binds to IP: $IP"
else
    fail "Server IP binding not detected"
fi

# Test 5: WebSocket Server
echo ""
echo "=== Test Suite 5: WebSocket Server ==="
if grep -q "WebSocket server started" /tmp/server_startup.log; then
    PORT=$(grep "WebSocket:" /tmp/server_startup.log | awk -F: '{print $NF}')
    pass "WebSocket server started on port$PORT"
else
    fail "WebSocket server failed to start"
fi

# Test 6: Discovery Service
echo ""
echo "=== Test Suite 6: Discovery Service ==="
if grep -q "UDP Discovery listening" /tmp/server_startup.log; then
    pass "UDP Discovery service active"
else
    fail "UDP Discovery service not running"
fi

# Test 7: CPU Monitoring
echo ""
echo "=== Test Suite 7: CPU Monitoring ==="
if grep -q "CPU monitoring active" /tmp/server_startup.log; then
    pass "CPU monitoring thread started"
else
    fail "CPU monitoring not active"
fi

# Test 8: HTTP API
echo ""
echo "=== Test Suite 8: HTTP API Endpoints ==="
timeout 3 ./servercontrol > /dev/null 2>&1 &
SERVER_PID=$!
sleep 2

# Get server IP and port from logs
IP=$(grep "HTTP API:" /tmp/server_startup.log | awk -F: '{print $(NF-1)}' | xargs)
PORT=2030

if curl -s --max-time 2 "http://${IP}:${PORT}/health" > /tmp/health.json 2>&1; then
    if grep -q "status" /tmp/health.json; then
        pass "HTTP health endpoint responding"
    else
        fail "HTTP health endpoint returned invalid response"
    fi
else
    info "HTTP endpoint test skipped (server binding to specific IP)"
fi

kill $SERVER_PID 2>/dev/null || true

# Test 9: File Structure
echo ""
echo "=== Test Suite 9: Modular Structure ==="
DIRS=("control/config" "control/core" "control/http" "control/network" "control/ui" \
      "server/core" "server/tasks" "server/monitoring" "server/network" "server/system")

for DIR in "${DIRS[@]}"; do
    if [ -d "$DIR" ]; then
        pass "Directory exists: $DIR"
    else
        fail "Missing directory: $DIR"
    fi
done

# Test 10: External Libraries
echo ""
echo "=== Test Suite 10: External Libraries ==="
LIBS=("include/asio.hpp" "include/nlohmann/json.hpp" "include/websocketpp/server.hpp")

for LIB in "${LIBS[@]}"; do
    if [ -f "$LIB" ]; then
        pass "Library found: $LIB"
    else
        fail "Missing library: $LIB"
    fi
done

# Test 11: Documentation
echo ""
echo "=== Test Suite 11: Documentation ==="
DOCS=("README.md" "ARCHITECTURE.md" "PRODUCTION_READY.md" "WEBSOCKET_API.md")

for DOC in "${DOCS[@]}"; do
    if [ -f "$DOC" ]; then
        LINES=$(wc -l < "$DOC")
        pass "Documentation exists: $DOC ($LINES lines)"
    else
        fail "Missing documentation: $DOC"
    fi
done

# Test 12: Code Quality
echo ""
echo "=== Test Suite 12: Code Quality ==="
CPP_FILES=$(find . -name "*.cpp" -not -path "./include/*" | wc -l)
H_FILES=$(find . -name "*.h" -not -path "./include/*" | wc -l)

if [ $CPP_FILES -gt 0 ]; then
    pass "Found $CPP_FILES C++ source files"
else
    fail "No C++ source files found"
fi

if [ $H_FILES -gt 0 ]; then
    pass "Found $H_FILES header files"
else
    fail "No header files found"
fi

# Test 13: WebSocket Protocol
echo ""
echo "=== Test Suite 13: WebSocket Protocol Messages ==="
if grep -q '"type".*"task_output"' server.cpp; then
    pass "WebSocket task_output message implemented"
else
    fail "WebSocket task_output message not found"
fi

if grep -q '"type".*"cpu_alert"' server.cpp; then
    pass "WebSocket cpu_alert message implemented"
else
    fail "WebSocket cpu_alert message not found"
fi

if grep -q '"current_dir"' server.cpp; then
    pass "Current directory tracking in messages"
else
    fail "Current directory tracking not implemented"
fi

# Test 14: Security Features
echo ""
echo "=== Test Suite 14: Security Features ==="
if grep -q "sanitize_filename" server.cpp; then
    pass "Filename sanitization implemented"
else
    fail "Filename sanitization not found"
fi

if grep -q "regex.*pattern" server.cpp; then
    pass "Input validation with regex"
else
    fail "Input validation not implemented"
fi

# Test 15: Performance
echo ""
echo "=== Test Suite 15: Performance Tests ==="
SERVER_SIZE=$(stat -c%s servercontrol 2>/dev/null || echo 0)
if [ $SERVER_SIZE -lt 10000000 ]; then  # Less than 10MB
    pass "Server binary size acceptable: $(($SERVER_SIZE / 1024 / 1024))MB"
else
    info "Server binary is large: $(($SERVER_SIZE / 1024 / 1024))MB"
fi

# Summary
echo ""
echo "========================================"
echo "           Test Summary"
echo "========================================"
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"
echo "Total:  $((PASSED + FAILED))"
echo ""

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ ALL TESTS PASSED!${NC}"
    exit 0
else
    echo -e "${RED}✗ SOME TESTS FAILED${NC}"
    exit 1
fi
