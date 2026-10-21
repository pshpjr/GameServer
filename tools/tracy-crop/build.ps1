param(
    [string]$TracySource,
    [string]$Installed,
    [string]$BuildDirectory
)
$ErrorActionPreference = 'Stop'
$workspace = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
if (!$Installed) { $Installed = Join-Path $workspace 'out/build/x64-Debug/vcpkg_installed/x64-windows' }
if (!$TracySource) {
    $sources = @(Get-ChildItem (Join-Path $workspace 'out/build/x64-Debug/vcpkg_installed/vcpkg/blds/tracy/src') -Directory -Filter 'v0.14.1-*.clean')
    if ($sources.Count -ne 1) { throw 'Specify -TracySource with a Tracy 0.14.1 source directory.' }
    $TracySource = $sources[0].FullName
}
if (!$BuildDirectory) { $BuildDirectory = Join-Path $workspace 'GameServer/out/build/tracy-crop' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'Visual Studio C++ build tools are required.' }
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64') {
    Import-Module (Join-Path $installation 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $installation -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
}
$cmake = Join-Path $installation 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$ninja = Join-Path $installation 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'
& $cmake -S $PSScriptRoot -B $BuildDirectory -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" '-DCMAKE_BUILD_TYPE=Release' "-DTRACY_SOURCE_DIR=$TracySource" "-DCMAKE_PREFIX_PATH=$Installed"
if ($LASTEXITCODE -ne 0) { throw 'Configuration failed.' }
& $cmake --build $BuildDirectory --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
Copy-Item -LiteralPath (Join-Path $Installed 'bin/zstd.dll') -Destination $BuildDirectory
$distribution = $PSScriptRoot
Copy-Item -LiteralPath (Join-Path $BuildDirectory 'tracy-crop.exe') -Destination $distribution
Copy-Item -LiteralPath (Join-Path $BuildDirectory 'zstd.dll') -Destination $distribution
Write-Output (Join-Path $distribution 'tracy-crop.exe')
