# Invoked by build.cmd on Windows PowerShell 5.1 or newer.
$ErrorActionPreference = 'Stop'
try {
    $lock = New-Object Threading.Mutex($false,'Local\GoldenEyeSourceBuilder')
    try { $locked = $lock.WaitOne(0) } catch [Threading.AbandonedMutexException] { $locked = $true }
    if (!$locked) { throw 'Another GoldenEye build is running. Let it finish, then retry.' }
    $root = Split-Path $PSScriptRoot
    $homeFolder = (Get-Item -LiteralPath $env:GE_BUILDER_HOME).FullName
    $interactive = !$env:GE_BUILD_TARGET
    $target = $env:GE_BUILD_TARGET
    if ($interactive) {
        Write-Host 'GoldenEye 007 - build on this PC'
        $choices = @{ '1' = 'windows'; '2' = 'xbox'; '3' = 'both' }
        do { $target = $choices[(Read-Host '1 Windows    2 Original Xbox    3 Both')] } until ($target)
    }
    $target = $target.ToLowerInvariant()
    if ($target -notin 'windows','xbox','both') { throw 'Expected windows, xbox or both.' }

    $inputs = @()
    foreach ($folder in @((Join-Path $homeFolder 'rom'), $homeFolder)) {
        if (Test-Path -LiteralPath $folder -PathType Container) {
            $inputs = @(Get-ChildItem -LiteralPath $folder -File | Where-Object { $_.Extension -in '.z64','.zip' })
            if ($inputs.Count) { break }
        }
    }
    if ($inputs.Count -eq 1) {
        $inputPath = $inputs[0].FullName
    } elseif ($interactive) {
        Add-Type -AssemblyName System.Windows.Forms
        $picker = New-Object Windows.Forms.OpenFileDialog
        $picker.Title = 'Choose your retail US GoldenEye ROM or ROM ZIP'
        $picker.Filter = 'GoldenEye ROM or ZIP|*.z64;*.zip'
        $picker.InitialDirectory = $homeFolder
        if ($picker.ShowDialog() -ne 'OK') { throw 'No ROM selected.' }
        $inputPath = $picker.FileName
        $picker.Dispose()
    } else {
        throw 'Place exactly one retail US .z64 or ROM ZIP in rom\ beside build.cmd.'
    }

    $size = 12 * 1024 * 1024
    $archive = $null
    $stream = $null
    try {
        if ([IO.Path]::GetExtension($inputPath) -ieq '.zip') {
            Add-Type -AssemblyName System.IO.Compression.FileSystem
            $archive = [IO.Compression.ZipFile]::OpenRead($inputPath)
            $members = @($archive.Entries | Where-Object { $_.Name -like '*.z64' -and $_.Length -eq $size })
            if ($members.Count -ne 1) { throw 'The ZIP must contain one 12 MiB .z64 file.' }
            $stream = $members[0].Open()
        } else {
            $stream = [IO.File]::OpenRead($inputPath)
            if ($stream.Length -ne $size) { throw 'Expected a 12 MiB retail US .z64 ROM.' }
        }
        $reader = New-Object IO.BinaryReader($stream)
        $bytes = $reader.ReadBytes($size)
        if ($bytes.Length -ne $size -or $stream.ReadByte() -ne -1) { throw 'Incorrect ROM size.' }
    } finally {
        if ($stream) { $stream.Dispose() }
        if ($archive) { $archive.Dispose() }
    }
    $expected = ((Get-Content -LiteralPath (Join-Path $root 'ge007.u.sha1') -Raw) -split '\s+')[0]
    $sha = [Security.Cryptography.SHA1]::Create()
    try { $hash = [BitConverter]::ToString($sha.ComputeHash($bytes)).Replace('-','') } finally { $sha.Dispose() }
    if ($hash -ne $expected) { throw 'ROM checksum mismatch. Use the unmodified retail US GoldenEye 007 .z64.' }
    Write-Host 'Retail US ROM verified.'
    $data = [IO.Directory]::CreateDirectory((Join-Path $root 'data'))
    $rom = Join-Path $data.FullName 'ge007.ntsc-final.z64'
    if (Test-Path -LiteralPath $rom) {
        $existing = [IO.File]::OpenRead($rom)
        $sha = [Security.Cryptography.SHA1]::Create()
        try { $existingHash = [BitConverter]::ToString($sha.ComputeHash($existing)).Replace('-','') } finally { $existing.Dispose(); $sha.Dispose() }
        if ($existingHash -ne $expected) { throw "An existing file differs from the verified ROM. Move it aside first: $rom" }
    } else {
        $tempRom = Join-Path $data.FullName ([guid]::NewGuid())
        try { [IO.File]::WriteAllBytes($tempRom,$bytes); [IO.File]::Move($tempRom,$rom) }
        finally { if (Test-Path -LiteralPath $tempRom) { Remove-Item -LiteralPath $tempRom } }
    }

    $msys = $env:GE_MSYS2_ROOT
    if (!$msys) { $msys = Join-Path $env:SystemDrive 'msys64' }
    $bash = Join-Path $msys 'usr\bin\bash.exe'
    if (!(Test-Path -LiteralPath $bash)) {
        Write-Host 'Installing the MSYS2 build tools using Windows Package Manager...'
        if ($env:GE_MSYS2_ROOT -or !(Get-Command winget.exe -ErrorAction SilentlyContinue)) {
            throw 'Install MSYS2 from https://www.msys2.org/ and rerun. Set GE_MSYS2_ROOT for a custom location.'
        }
        & winget.exe install --id MSYS2.MSYS2 --exact --source winget --location $msys
        if (!(Test-Path -LiteralPath $bash)) { throw 'Finish installing MSYS2 and rerun build.cmd.' }
    }
    $env:GE_BUILD_TARGET = $target
    $env:GE_MSYS2_ROOT = $msys
    $env:MSYSTEM = 'MINGW64'
    $env:PATH = "$(Join-Path $msys 'usr\bin');$(Join-Path $msys 'mingw64\bin');$env:PATH"
    Set-Location -LiteralPath $root
    & $bash 'tools_pc/build-local.sh'
    if ($LASTEXITCODE -ne 0) { throw 'Build tools reported an error.' }
    $folders = @()
    if ($target -ne 'xbox') { $folders += (Join-Path $root 'build-pc\play') }
    if ($target -ne 'windows') { $folders += (Join-Path $root 'build-xbox\disc') }
    foreach ($folder in $folders) {
        Write-Host "Ready: $folder"
        if ($interactive) { Invoke-Item -LiteralPath $folder }
    }
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
} finally {
    if ($locked) { $lock.ReleaseMutex() }
    if ($lock) { $lock.Dispose() }
}
