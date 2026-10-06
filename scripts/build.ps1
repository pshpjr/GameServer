[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$BuildDirectory,
    [string]$InstalledDirectory,
    [switch]$Tracy
)
$ErrorActionPreference = 'Stop'
$sourceRoot = Split-Path $PSScriptRoot -Parent
if (-not $VcpkgRoot) { throw 'Specify -VcpkgRoot or set VCPKG_ROOT.' }
$VcpkgRoot = (Resolve-Path -LiteralPath $VcpkgRoot).Path
$toolchain = Join-Path $VcpkgRoot 'scripts/buildsystems/vcpkg.cmake'
if (-not (Test-Path -LiteralPath $toolchain)) { throw "Missing toolchain: $toolchain" }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsPath = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) { throw 'Visual Studio C++ x64 tools are required.' }
Import-Module (Join-Path $vsPath 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
if (-not $BuildDirectory) { $BuildDirectory = Join-Path $sourceRoot "../out/build/verified-$Configuration" }
$tracyValue = if ($Tracy) { 'ON' } else { 'OFF' }
$configureArgs = @('-S', $sourceRoot, '-B', $BuildDirectory, '-G', 'Ninja',
    "-DCMAKE_BUILD_TYPE=$Configuration", "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
    '-DVCPKG_TARGET_TRIPLET=x64-windows',
    "-DNETLIB_ENABLE_TRACY=$tracyValue", '-DVCPKG_MANIFEST_INSTALL=ON')
if ($InstalledDirectory) {
    $InstalledDirectory = (Resolve-Path -LiteralPath $InstalledDirectory).Path
    $configureArgs += @("-DVCPKG_INSTALLED_DIR=$InstalledDirectory", '-DVCPKG_MANIFEST_INSTALL=OFF')
}
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
& cmake --build $BuildDirectory --parallel 8
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
