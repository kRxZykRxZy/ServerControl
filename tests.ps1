# ServerControl Test Suite (PowerShell)
# Comprehensive testing for Windows deployment

$ErrorActionPreference = "Continue"

$passed = 0
$failed = 0

function Test-Pass { 
    param([string]$Message)
    Write-Host "✓ PASS: $Message" -ForegroundColor Green
    $script:passed++
}

function Test-Fail { 
    param([string]$Message)
    Write-Host "✗ FAIL: $Message" -ForegroundColor Red
    $script:failed++
}

function Test-Info { 
    param([string]$Message)
    Write-Host "ℹ INFO: $Message" -ForegroundColor Cyan
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  ServerControl Test Suite (Windows)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Test 1: File existence
Write-Host "=== Test Suite 1: Binary Files ===" -ForegroundColor Yellow

if (Test-Path ".\files\servercontrol.exe") {
    $size = (Get-Item ".\files\servercontrol.exe").Length / 1MB
    Test-Pass ("Server binary exists ({0:N2} MB)" -f $size)
} else {
    if (Test-Path ".\files\servercontrol") {
        $size = (Get-Item ".\files\servercontrol").Length / 1MB
        Test-Pass ("Server binary exists (Linux format, {0:N2} MB)" -f $size)
    } else {
        Test-Fail "Server binary not found"
    }
}

if (Test-Path ".\files\control.exe") {
    $size = (Get-Item ".\files\control.exe").Length / 1MB
    Test-Pass ("Control binary exists ({0:N2} MB)" -f $size)
} else {
    if (Test-Path ".\files\control") {
        $size = (Get-Item ".\files\control").Length / 1MB
        Test-Pass ("Control binary exists (Linux format, {0:N2} MB)" -f $size)
    } else {
        Test-Fail "Control binary not found"
    }
}

# Test 2: Directory structure
Write-Host ""
Write-Host "=== Test Suite 2: Directory Structure ===" -ForegroundColor Yellow

$dirs = @(
    "control\config",
    "control\core",
    "control\http",
    "control\network",
    "control\ui",
    "server\core",
    "server\tasks",
    "server\monitoring",
    "server\network",
    "server\system"
)

foreach ($dir in $dirs) {
    if (Test-Path $dir) {
        Test-Pass "Directory exists: $dir"
    } else {
        Test-Fail "Missing directory: $dir"
    }
}

# Test 3: External libraries
Write-Host ""
Write-Host "=== Test Suite 3: External Libraries ===" -ForegroundColor Yellow

$libs = @(
    "include\asio.hpp",
    "include\nlohmann\json.hpp",
    "include\websocketpp\server.hpp"
)

foreach ($lib in $libs) {
    if (Test-Path $lib) {
        Test-Pass "Library found: $lib"
    } else {
        Test-Fail "Missing library: $lib"
    }
}

# Test 4: Build scripts
Write-Host ""
Write-Host "=== Test Suite 4: Build Scripts ===" -ForegroundColor Yellow

$scripts = @(
    "build.bat",
    "build.ps1",
    "build-all.ps1",
    "deploy.ps1",
    "CMakeLists.txt"
)

foreach ($script in $scripts) {
    if (Test-Path $script) {
        Test-Pass "Build script exists: $script"
    } else {
        Test-Fail "Missing script: $script"
    }
}

# Test 5: Documentation
Write-Host ""
Write-Host "=== Test Suite 5: Documentation ===" -ForegroundColor Yellow

$docs = @(
    "README.md",
    "ARCHITECTURE.md",
    "PRODUCTION_READY.md",
    "WEBSOCKET_API.md"
)

foreach ($doc in $docs) {
    if (Test-Path $doc) {
        $lines = (Get-Content $doc).Count
        Test-Pass "Documentation exists: $doc ($lines lines)"
    } else {
        Test-Fail "Missing documentation: $doc"
    }
}

# Test 6: Code files
Write-Host ""
Write-Host "=== Test Suite 6: Source Code ===" -ForegroundColor Yellow

$cppFiles = Get-ChildItem -Recurse -Include *.cpp -Exclude include | Measure-Object
$hFiles = Get-ChildItem -Recurse -Include *.h -Exclude include | Measure-Object

if ($cppFiles.Count -gt 0) {
    Test-Pass "Found $($cppFiles.Count) C++ source files"
} else {
    Test-Fail "No C++ source files found"
}

if ($hFiles.Count -gt 0) {
    Test-Pass "Found $($hFiles.Count) header files"
} else {
    Test-Fail "No header files found"
}

# Test 7: Features in code
Write-Host ""
Write-Host "=== Test Suite 7: Feature Implementation ===" -ForegroundColor Yellow

$serverContent = Get-Content "server.cpp" -Raw

if ($serverContent -match '"type".*"task_output"') {
    Test-Pass "WebSocket task_output implemented"
} else {
    Test-Fail "WebSocket task_output not found"
}

if ($serverContent -match '"current_dir"') {
    Test-Pass "Current directory tracking implemented"
} else {
    Test-Fail "Current directory tracking not found"
}

if ($serverContent -match 'sanitize_filename') {
    Test-Pass "Filename sanitization implemented"
} else {
    Test-Fail "Filename sanitization not found"
}

# Summary
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Test Summary" -ForegroundColor White
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Passed: $passed" -ForegroundColor Green
Write-Host "Failed: $failed" -ForegroundColor Red
Write-Host "Total:  $($passed + $failed)" -ForegroundColor White
Write-Host ""

if ($failed -eq 0) {
    Write-Host "✓ ALL TESTS PASSED!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "✗ SOME TESTS FAILED" -ForegroundColor Red
    exit 1
}
