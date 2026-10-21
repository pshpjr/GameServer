[CmdletBinding()]
param(
    [ValidateSet('build','create','start','stop','restart','status','logs')]
    [string]$Action = 'status',
    [ValidateSet('mysql','gameserver','dummy','dummy2','otel-collector','prometheus','grafana')]
    [string]$Service
)
$ErrorActionPreference = 'Stop'
if ($Action -in @('start','stop','restart','logs') -and -not $Service) {
    throw 'Select one service: mysql, gameserver, dummy, dummy2, otel-collector, prometheus, or grafana.'
}
Push-Location $PSScriptRoot
try {
    $arguments = @('compose', '--env-file', '.env', '-f', 'compose.yaml')
    switch ($Action) {
        'status' { $arguments += 'ps' }
        'create' { $arguments += @('create', '--no-build') }
        default { $arguments += $Action }
    }
    if ($Service) { $arguments += $Service }
    & docker @arguments
    if ($LASTEXITCODE -ne 0) { throw "Docker Compose failed: $Action" }
} finally { Pop-Location }
