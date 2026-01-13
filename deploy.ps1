# ServerControl Deployment Script for Windows
# Full PowerShell support for enterprise deployment

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Server", "Control", "Both")]
    [string]$Component = "Both",
    
    [Parameter(Mandatory=$false)]
    [string]$InstallPath = "C:\ServerControl",
    
    [Parameter(Mandatory=$false)]
    [switch]$Service,
    
    [Parameter(Mandatory=$false)]
    [switch]$AutoStart
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  ServerControl Deployment" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check administrator privileges
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin -and $Service) {
    Write-Host "ERROR: Administrator privileges required for service installation" -ForegroundColor Red
    Write-Host "Please run PowerShell as Administrator" -ForegroundColor Yellow
    exit 1
}

# Create installation directory
Write-Host "Creating installation directory: $InstallPath" -ForegroundColor Yellow
New-Item -ItemType Directory -Force -Path $InstallPath | Out-Null

# Copy files
if ($Component -eq "Server" -or $Component -eq "Both") {
    Write-Host "Installing Server component..." -ForegroundColor Green
    Copy-Item ".\files\servercontrol" "$InstallPath\servercontrol.exe" -Force
    Write-Host "  ✓ servercontrol.exe installed" -ForegroundColor Green
}

if ($Component -eq "Control" -or $Component -eq "Both") {
    Write-Host "Installing Control component..." -ForegroundColor Green
    Copy-Item ".\files\control" "$InstallPath\control.exe" -Force
    Write-Host "  ✓ control.exe installed" -ForegroundColor Green
}

# Install as Windows Service
if ($Service -and ($Component -eq "Server" -or $Component -eq "Both")) {
    Write-Host ""
    Write-Host "Installing Windows Service..." -ForegroundColor Yellow
    
    $serviceName = "ServerControl"
    $serviceDisplayName = "ServerControl Server"
    $serviceDescription = "ServerControl remote management server"
    $servicePath = "$InstallPath\servercontrol.exe"
    
    # Check if service exists
    $existingService = Get-Service -Name $serviceName -ErrorAction SilentlyContinue
    
    if ($existingService) {
        Write-Host "  Service already exists, stopping..." -ForegroundColor Yellow
        Stop-Service -Name $serviceName -Force
        Start-Sleep -Seconds 2
        
        # Delete existing service
        sc.exe delete $serviceName | Out-Null
        Start-Sleep -Seconds 2
    }
    
    # Create new service
    New-Service -Name $serviceName `
                -BinaryPathName $servicePath `
                -DisplayName $serviceDisplayName `
                -Description $serviceDescription `
                -StartupType Automatic | Out-Null
    
    Write-Host "  ✓ Service installed: $serviceName" -ForegroundColor Green
    
    if ($AutoStart) {
        Write-Host "  Starting service..." -ForegroundColor Yellow
        Start-Service -Name $serviceName
        Write-Host "  ✓ Service started" -ForegroundColor Green
    }
}

# Create desktop shortcuts
Write-Host ""
Write-Host "Creating shortcuts..." -ForegroundColor Yellow

$WshShell = New-Object -comObject WScript.Shell

if ($Component -eq "Control" -or $Component -eq "Both") {
    $Shortcut = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\ServerControl.lnk")
    $Shortcut.TargetPath = "$InstallPath\control.exe"
    $Shortcut.WorkingDirectory = $InstallPath
    $Shortcut.Description = "ServerControl Management Console"
    $Shortcut.Save()
    Write-Host "  ✓ Desktop shortcut created" -ForegroundColor Green
}

# Add to PATH
Write-Host ""
Write-Host "Adding to system PATH..." -ForegroundColor Yellow
$currentPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($currentPath -notlike "*$InstallPath*") {
    [Environment]::SetEnvironmentVariable("Path", "$currentPath;$InstallPath", "User")
    Write-Host "  ✓ Added to PATH" -ForegroundColor Green
} else {
    Write-Host "  ℹ Already in PATH" -ForegroundColor Gray
}

# Firewall rules
if ($isAdmin -and ($Component -eq "Server" -or $Component -eq "Both")) {
    Write-Host ""
    Write-Host "Configuring Windows Firewall..." -ForegroundColor Yellow
    
    # Remove existing rules
    Remove-NetFirewallRule -DisplayName "ServerControl*" -ErrorAction SilentlyContinue
    
    # Add new rules
    New-NetFirewallRule -DisplayName "ServerControl HTTP" `
                        -Direction Inbound `
                        -Protocol TCP `
                        -LocalPort 2030 `
                        -Action Allow `
                        -Profile Any | Out-Null
    
    New-NetFirewallRule -DisplayName "ServerControl WebSocket" `
                        -Direction Inbound `
                        -Protocol TCP `
                        -LocalPort 2032 `
                        -Action Allow `
                        -Profile Any | Out-Null
    
    New-NetFirewallRule -DisplayName "ServerControl Discovery" `
                        -Direction Inbound `
                        -Protocol UDP `
                        -LocalPort 2031 `
                        -Action Allow `
                        -Profile Any | Out-Null
    
    Write-Host "  ✓ Firewall rules configured" -ForegroundColor Green
}

# Summary
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Installation Complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Installation Path: $InstallPath" -ForegroundColor White
Write-Host ""

if ($Component -eq "Server" -or $Component -eq "Both") {
    Write-Host "Server Component:" -ForegroundColor Cyan
    Write-Host "  Executable: $InstallPath\servercontrol.exe" -ForegroundColor White
    if ($Service) {
        Write-Host "  Service: $serviceName" -ForegroundColor White
        Write-Host "  Status: " -NoNewline -ForegroundColor White
        $svc = Get-Service -Name $serviceName
        if ($svc.Status -eq "Running") {
            Write-Host "Running" -ForegroundColor Green
        } else {
            Write-Host "Stopped" -ForegroundColor Yellow
        }
    }
    Write-Host ""
}

if ($Component -eq "Control" -or $Component -eq "Both") {
    Write-Host "Control Component:" -ForegroundColor Cyan
    Write-Host "  Executable: $InstallPath\control.exe" -ForegroundColor White
    Write-Host "  Shortcut: Desktop\ServerControl.lnk" -ForegroundColor White
    Write-Host ""
}

Write-Host "Usage:" -ForegroundColor Yellow
if ($Component -eq "Server" -or $Component -eq "Both") {
    if ($Service) {
        Write-Host "  Start-Service ServerControl     # Start server" -ForegroundColor Gray
        Write-Host "  Stop-Service ServerControl      # Stop server" -ForegroundColor Gray
    } else {
        Write-Host "  servercontrol.exe               # Start server" -ForegroundColor Gray
    }
}
if ($Component -eq "Control" -or $Component -eq "Both") {
    Write-Host "  control.exe                     # Open control panel" -ForegroundColor Gray
}
Write-Host ""
