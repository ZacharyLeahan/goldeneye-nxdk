param(
    [Parameter(Mandatory = $true)][string]$GameDirectory,
    [string]$Zig = "zig",
    [string]$Python = "python"
)
$ErrorActionPreference = "Stop"
$gamePath = (Resolve-Path -LiteralPath $GameDirectory).Path
$gameExe = Join-Path $gamePath "GE32.exe"
if (-not (Test-Path -LiteralPath $gameExe)) { throw "GE32.exe was not found in GameDirectory." }
$runningGame = Get-Process -Name GE32 -ErrorAction SilentlyContinue
if ($runningGame) { throw "Close GE32 before installing the extension." }
$buildPath = Join-Path $PSScriptRoot "build"
New-Item -ItemType Directory -Path $buildPath -Force | Out-Null
$dll = Join-Path $buildPath "weapon-wheel.dll"
& $Zig cc -shared -O2 -Wall -Wextra -o $dll (Join-Path $PSScriptRoot "weapon-wheel.c") -lopengl32 -lgdi32 -luser32
if ($LASTEXITCODE -ne 0) { throw "Extension compilation failed." }
& $Python (Join-Path $PSScriptRoot "patch-character-back.py") $gameExe
if ($LASTEXITCODE -ne 0) { throw "Character-screen patch failed; unsupported executable or invalid layout." }
& $Python (Join-Path $PSScriptRoot "install-wheel.py") $gameExe
if ($LASTEXITCODE -ne 0) { throw "Extension loader installation failed." }
Copy-Item -LiteralPath $dll -Destination (Join-Path $gamePath "weapon-wheel.dll") -Force
& $Python (Join-Path $PSScriptRoot "apply-controls.py") (Join-Path $gamePath "data/ge007.ini")
if ($LASTEXITCODE -ne 0) { throw "Controller profile installation failed." }
Write-Output "Installed. Start GE32.exe from its game directory."
