$ErrorActionPreference = "Stop"
& "$env:USERPROFILE\zephyrproject\.venv\Scripts\Activate.ps1"
python -m serial.tools.miniterm COM9 115200 --raw
