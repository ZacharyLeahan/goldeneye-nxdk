@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "GE_BUILDER_HOME=%~dp0."
set "GE_BUILD_TARGET=%~1"
set "GE_BUILDER_TAG=builder-1.0"
if "%~1"=="/?" goto help
if /i "%~1"=="--help" goto help
if not "%~2"=="" goto usage
if "%~1"=="" goto build
for %%t in (windows xbox both) do if /i "%~1"=="%%t" goto build
:usage
echo Usage: build.cmd [windows^|xbox^|both]
exit /b 2
:help
echo GoldenEye local source builder. Place your retail US .z64 or ROM ZIP beside this file or inside rom\.
echo Run build.cmd for the menu, or use build.cmd windows, build.cmd xbox, or build.cmd both.
exit /b 0
:build
set "source=%~dp0."
if exist "%source%\tools_pc\build-local.ps1" if exist "%source%\port\xbox\build.py" goto ready
set "source=%~dp0GoldenEye-%GE_BUILDER_TAG%"
if exist "%source%\tools_pc\build-local.ps1" goto ready
echo Downloading the matching GoldenEye source from ZacharyLeahan/goldeneye-nxdk...
powershell.exe -NoProfile -Command ^
 "$ErrorActionPreference='Stop'; $ProgressPreference='SilentlyContinue'; $tmp=New-Item -ItemType Directory -Path (Join-Path $env:GE_BUILDER_HOME ([guid]::NewGuid())); $archive=Join-Path $tmp.FullName 'source.zip';" ^
 "try { [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12; curl.exe --fail --location --retry 2 --output $archive ('https://codeload.github.com/ZacharyLeahan/goldeneye-nxdk/zip/refs/tags/'+$env:GE_BUILDER_TAG); if ($LASTEXITCODE) { throw 'Source download failed.' }; tar.exe -xf $archive -C $tmp.FullName; if ($LASTEXITCODE) { throw 'Source extraction failed.' }; $tree=Join-Path $tmp.FullName ('goldeneye-nxdk-'+$env:GE_BUILDER_TAG); if (!(Test-Path -LiteralPath (Join-Path $tree 'tools_pc/build-local.ps1'))) { throw 'Incomplete source download.' }; [IO.Directory]::Move($tree,$env:source) } finally { Remove-Item -LiteralPath $tmp.FullName -Recurse -Force }"
if errorlevel 1 goto failed
:ready
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%source%\tools_pc\build-local.ps1"
if errorlevel 1 goto failed
if "%~1"=="" pause
exit /b 0
:failed
echo Build did not complete. See the message above, then run build.cmd again.
if "%~1"=="" pause
exit /b 1
