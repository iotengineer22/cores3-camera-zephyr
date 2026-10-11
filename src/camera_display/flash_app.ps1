$ErrorActionPreference = "Stop"
& "$env:USERPROFILE\zephyrproject\.venv\Scripts\Activate.ps1"
$env:ZEPHYR_BASE = "$env:USERPROFILE\zephyrproject\zephyr"
Set-Location "X:\src\camera_display"
west -z "$env:ZEPHYR_BASE" flash -d "X:\src\camera_display\build" --esp-device COM9
