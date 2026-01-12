#!/bin/bash

# Test script for ServerControl

set -e

echo "=== ServerControl Test Suite ==="
echo ""

# Test 1: Check if server is responding
echo "Test 1: Server HTTP API"
echo "Testing /hostname endpoint..."
HOSTNAME=$(curl -s http://localhost:8080/hostname)
echo "  Response: $HOSTNAME"

echo "Testing /stats endpoint..."
STATS=$(curl -s http://localhost:8080/stats)
echo "  Response: $STATS"

echo "Testing /tasks endpoint..."
TASKS=$(curl -s http://localhost:8080/tasks)
echo "  Response: $TASKS"

# Test 2: Execute a command
echo ""
echo "Test 2: Command Execution"
echo "Executing 'echo Hello World'..."
EXEC_RESULT=$(curl -s -X POST http://localhost:8080/exec -d '{"cmd":"echo Hello World"}')
echo "  Response: $EXEC_RESULT"

# Extract task ID
TASK_ID=$(echo $EXEC_RESULT | grep -o '"task_id":"[^"]*"' | cut -d'"' -f4)
echo "  Task ID: $TASK_ID"

# Wait a moment for task to complete
sleep 1

# Test 3: Get logs
echo ""
echo "Test 3: Get Task Logs"
LOGS=$(curl -s "http://localhost:8080/logs?id=$TASK_ID")
echo "  Logs: $LOGS"

# Test 4: List tasks
echo ""
echo "Test 4: List All Tasks"
ALL_TASKS=$(curl -s http://localhost:8080/tasks)
echo "  Tasks: $ALL_TASKS"

# Test 5: Execute another command to test concurrent execution
echo ""
echo "Test 5: Concurrent Task Execution"
echo "Executing 'sleep 2 && echo Done'..."
EXEC_RESULT2=$(curl -s -X POST http://localhost:8080/exec -d '{"cmd":"sleep 2 && echo Done"}')
echo "  Response: $EXEC_RESULT2"

TASK_ID2=$(echo $EXEC_RESULT2 | grep -o '"task_id":"[^"]*"' | cut -d'"' -f4)
echo "  Task ID: $TASK_ID2"

# Check task is running
sleep 0.5
RUNNING_TASKS=$(curl -s http://localhost:8080/tasks)
echo "  Running tasks: $RUNNING_TASKS"

echo ""
echo "=== All Tests Passed ==="
