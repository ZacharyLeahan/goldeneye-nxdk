@echo off
setlocal
cd /d "%~dp0"
if not exist data\ge007.ntsc-final.z64 (
  echo Place your retail US ROM in data\ge007.ntsc-final.z64, then run this again.
  pause
  exit /b 1
)
if not exist data\pcmodels-ntsc-final\pcmodels.bin goto prepare
if not exist data\pccg-ntsc-final\pccg.bin goto prepare
goto host
:prepare
if exist prepare-assets\ge007-convert.exe (
  prepare-assets\ge007-convert.exe --rom data\ge007.ntsc-final.z64
  if errorlevel 1 exit /b 1
  goto host
)
where py >nul 2>nul
if errorlevel 1 (
  echo First-time asset preparation requires Python 3 and the Windows py launcher.
  pause
  exit /b 1
)
py -3 prepare-assets\prepare-assets.py --rom data\ge007.ntsc-final.z64
if errorlevel 1 (
  pause
  exit /b 1
)
:host
echo Enter the wired Ethernet IPv4 address on the Xbox.
ipconfig
echo Starting two-player Complex on UDP 27007. Waiting for the Xbox to join.
"%~dp0ge007.x86_64.exe" --lan-host
if errorlevel 1 pause
