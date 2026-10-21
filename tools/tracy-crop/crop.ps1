param(
    [Parameter(Mandatory, Position=0)][string]$InputFile,
    [Parameter(Mandatory, Position=1)][string]$OutputFile,
    [Parameter(Mandatory, Position=2)][string]$Start,
    [Parameter(Mandatory, Position=3)][string]$End
)
$ErrorActionPreference = 'Stop'
$executable = Join-Path $PSScriptRoot 'tracy-crop.exe'
if (!(Test-Path -LiteralPath $executable)) { & (Join-Path $PSScriptRoot 'build.ps1') }
& $executable $InputFile $OutputFile $Start $End
if ($LASTEXITCODE -ne 0) { throw "Tracy extraction failed (exit $LASTEXITCODE)." }
