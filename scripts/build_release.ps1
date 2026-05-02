param(
    [string]$SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$BuildDir = "build\release-nmake",
    [switch]$SkipPackage
)

$ErrorActionPreference = "Stop"

$cmake = "E:\Coding\Cmake\bin\cmake.exe"
$msvcRoot = "E:\Coding\TypeC\VC\Tools\MSVC\14.50.35717"
$sdkRoot = "C:\Program Files (x86)\Windows Kits\10"
$sdkVersion = "10.0.26100.0"

$cl = Join-Path $msvcRoot "bin\Hostx64\x64\cl.exe"
$nmake = Join-Path $msvcRoot "bin\Hostx64\x64\nmake.exe"
$rc = Join-Path $sdkRoot "bin\$sdkVersion\x64\rc.exe"

$buildPath = Join-Path $SourceRoot $BuildDir
$zipPath = Join-Path $SourceRoot "bin\MulNX_release.zip"
$dllPath = Join-Path $SourceRoot "bin\MulNX\MulNX\DLLToCS\CS2OBTool.dll"
$injectorPath = Join-Path $SourceRoot "bin\MulNX\MulNX\CS2Injector.exe"

function Invoke-NativeCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(Mandatory = $false)]
        [string[]]$Arguments = @()
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "命令执行失败: $FilePath $($Arguments -join ' ')"
    }
}

foreach ($tool in @($cmake, $cl, $nmake, $rc)) {
    if (-not (Test-Path $tool)) {
        throw "缺少必要工具: $tool"
    }
}

$env:PATH = "$($msvcRoot)\bin\Hostx64\x64;$sdkRoot\bin\$sdkVersion\x64;$env:PATH"
$env:INCLUDE = "$($msvcRoot)\include;$sdkRoot\Include\$sdkVersion\ucrt;$sdkRoot\Include\$sdkVersion\shared;$sdkRoot\Include\$sdkVersion\um;$sdkRoot\Include\$sdkVersion\winrt;$sdkRoot\Include\$sdkVersion\cppwinrt"
$env:LIB = "$($msvcRoot)\lib\x64;$sdkRoot\Lib\$sdkVersion\ucrt\x64;$sdkRoot\Lib\$sdkVersion\um\x64"

Invoke-NativeCommand -FilePath $cmake -Arguments @(
    "-S", $SourceRoot,
    "-B", $buildPath,
    "-G", "NMake Makefiles",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_C_COMPILER=$cl",
    "-DCMAKE_CXX_COMPILER=$cl",
    "-DCMAKE_MAKE_PROGRAM=$nmake",
    "-DCMAKE_RC_COMPILER=$rc"
)

Invoke-NativeCommand -FilePath $cmake -Arguments @(
    "--build", $buildPath,
    "--config", "Release"
)

if (-not (Test-Path $dllPath)) {
    throw "未找到构建产物: $dllPath"
}
if (-not (Test-Path $injectorPath)) {
    throw "未找到构建产物: $injectorPath"
}

if (-not $SkipPackage) {
    if (Test-Path $zipPath) {
        Remove-Item $zipPath -Force
    }
    Compress-Archive -Path (Join-Path $SourceRoot "bin\MulNX\*") -DestinationPath $zipPath -Force
}

Write-Host "构建完成"
Write-Host "DLL: $dllPath"
Write-Host "Injector: $injectorPath"
if (-not $SkipPackage) {
    Write-Host "Zip: $zipPath"
}
