# 세 서버(인프라 1.1, 더미 1.3, 게임 서버 1.5) 제어 스크립트. 설정은 servers.psd1.
#   build    [-Role r]  이미지 빌드 (Windows Docker Desktop)
#   transfer [-Role r]  docker save → scp → docker load, compose·설정 전송
#   start    [-Role r]  역할 없이: 인프라 → 게임 서버 → 더미 순서로 시작하고 각각 준비를 확인
#   stop     [-Role r]  역할 없이: 더미 → 게임 서버 순서로 종료. 인프라는 -Role infra로만 끈다.
#   restart  -Role r    해당 역할만 다시 시작
#   status              세 서버의 컨테이너 상태
#   deploy   [-Role r]  build → transfer → start
# 실패하면 그 자리에서 멈추고 "[단계] 역할(주소)"를 표시한다.
[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet('build', 'transfer', 'start', 'stop', 'restart', 'status', 'deploy')]
    [string]$Action = 'status',
    [ValidateSet('infra', 'gameserver', 'dummy')]
    [string]$Role
)
$ErrorActionPreference = 'Stop'
$config = Import-PowerShellDataFile (Join-Path $PSScriptRoot 'servers.psd1')
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$roles = if ($Role) { @($Role) } else { @('infra', 'gameserver', 'dummy') }

function Fail([string]$step, [string]$role, [string]$message) {
    $where = if ($role) { " $role($($config.Hosts[$role]))" } else { '' }
    throw "[$step]$where 실패: $message"
}

function Remote([string]$role, [string]$command, [string]$step) {
    $target = "$($config.User)@$($config.Hosts[$role])"
    $output = & ssh -o BatchMode=yes -o ConnectTimeout=10 $target $command 2>&1
    if ($LASTEXITCODE -ne 0) { Fail $step $role ($output | Out-String).Trim() }
    return $output
}

function Compose([string]$role, [string]$arguments, [string]$step) {
    Remote $role "cd $($config.Root)/$role && docker compose --env-file ../.env $arguments" $step
}

#  비밀값은 처음 한 번 만들고 secrets.env에 둔다. 로컬 자격 증명은 쓰지 않는다.
function Secrets {
    $path = Join-Path $PSScriptRoot 'secrets.env'
    if (-not (Test-Path $path)) {
        $new = { -join ((48..57) + (65..90) + (97..122) | Get-Random -Count 24 | ForEach-Object { [char]$_ }) }
        @(
            "MYSQL_ROOT_PASSWORD=$(& $new)"
            "MYSQL_PASSWORD=$(& $new)"
            "MYSQL_MONITOR_PASSWORD=$(& $new)"
        ) | Set-Content -Encoding ascii $path
    }
    $values = @{}
    foreach ($line in Get-Content $path) {
        if ($line -match '^([A-Z_]+)=(.*)$') { $values[$Matches[1]] = $Matches[2] }
    }
    return $values
}

#  역할별로 서버에 보낼 파일을 .stage/<역할>에 모은다.
function Stage([string]$role) {
    $secrets = Secrets
    $stage = Join-Path $PSScriptRoot ".stage/$role"
    if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
    New-Item -ItemType Directory -Force $stage | Out-Null
    Copy-Item -Recurse (Join-Path $PSScriptRoot "$role/*") $stage
    switch ($role) {
        'infra' {
            $dashboardDirectory = Join-Path $stage 'config/grafana/dashboards'
            New-Item -ItemType Directory -Force $dashboardDirectory | Out-Null
            Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../local/config/grafana/dashboards/gameserver.json') -Destination $dashboardDirectory
            @(
                "MYSQL_ROOT_PASSWORD=$($secrets.MYSQL_ROOT_PASSWORD)"
                'MYSQL_DATABASE=mydb'
                'MYSQL_USER=game'
                "MYSQL_PASSWORD=$($secrets.MYSQL_PASSWORD)"
                'MYSQL_MONITOR_USER=otel_monitor'
                "MYSQL_MONITOR_PASSWORD=$($secrets.MYSQL_MONITOR_PASSWORD)"
            ) | Set-Content -Encoding ascii (Join-Path $stage 'infra.env')
        }
        'gameserver' {
            $template = Join-Path $stage 'config/GameSettings.template.toml'
            (Get-Content -Raw $template).Replace('{{INFRA_HOST}}', $config.Hosts.infra).Replace('{{MYSQL_PASSWORD}}', $secrets.MYSQL_PASSWORD) |
                Set-Content -Encoding utf8NoBOM -NoNewline (Join-Path $stage 'config/GameSettings.toml')
            Remove-Item $template
        }
    }
    #  compose가 읽는 공통 값. 역할 폴더의 상위(~/portfolio26/.env)에 둔다.
    @(
        "INFRA_IMAGE=$($config.Images.infra)"
        "GAMESERVER_IMAGE=$($config.Images.gameserver)"
        "DUMMY_IMAGE=$($config.Images.dummy)"
        "INFRA_HOST=$($config.Hosts.infra)"
    ) | Set-Content -Encoding ascii (Join-Path $PSScriptRoot '.stage/.env')
    return $stage
}

function BuildRole([string]$role) {
    Push-Location $repo
    try {
        if ($role -eq 'infra') {
            & docker build -f deploy/servers/infra/Dockerfile -t $config.Images.infra deploy
        } else {
            $target = if ($role -eq 'dummy') { 'local-dummy' } else { 'local-gameserver' }
            & docker buildx build --load --target $target -t $config.Images[$role] `
                --build-arg "BUILD_PRESET=$($config.BuildPreset)" `
                --build-arg "VCPKG_CACHE_IMAGE=$($config.VcpkgCacheImage)" `
                --build-arg "NETLIB_ENABLE_OTEL=$($config.EnableOtel)" `
                --build-arg "NETLIB_ENABLE_TRACY=$($config.EnableTracy)" .
        }
        if ($LASTEXITCODE -ne 0) { Fail 'build' $role "docker build 종료 코드 $LASTEXITCODE" }
    } finally { Pop-Location }
}

function TransferRole([string]$role) {
    $image = $config.Images[$role]
    $imageId = (& docker image inspect --format '{{.Id}}' $image)
    if ($LASTEXITCODE -ne 0) { Fail 'transfer' $role "로컬에 이미지 $image 가 없다. build를 먼저 한다." }

    #  서버에 같은 이미지가 이미 있으면 보내지 않는다.
    $remoteId = Remote $role "docker image inspect --format '{{.Id}}' $image 2>/dev/null || true" 'transfer'
    if (($remoteId | Out-String).Trim() -ne $imageId) {
        $images = Join-Path $PSScriptRoot 'images'
        New-Item -ItemType Directory -Force $images | Out-Null
        $file = Join-Path $images ("$role-" + $imageId.Substring(7, 12) + '.tar')
        if (-not (Test-Path $file)) {
            & docker save -o $file $image
            if ($LASTEXITCODE -ne 0) { Fail 'transfer' $role 'docker save' }
        }
        Remote $role "mkdir -p $($config.Root)/images" 'transfer' | Out-Null
        & scp -q -o BatchMode=yes $file "$($config.User)@$($config.Hosts[$role]):$($config.Root)/images/"
        if ($LASTEXITCODE -ne 0) { Fail 'transfer' $role 'scp 이미지' }
        Remote $role "docker load -i $($config.Root)/images/$(Split-Path -Leaf $file) && rm -f $($config.Root)/images/$(Split-Path -Leaf $file)" 'transfer' | Out-Null
    }

    $stage = Stage $role
    Remote $role "mkdir -p $($config.Root)/$role" 'transfer' | Out-Null
    & scp -q -r -o BatchMode=yes "$stage/*" (Join-Path $PSScriptRoot '.stage/.env') (Join-Path $PSScriptRoot '../local/seccomp-io-uring.json') `
        "$($config.User)@$($config.Hosts[$role]):$($config.Root)/$role/"
    if ($LASTEXITCODE -ne 0) { Fail 'transfer' $role 'scp 설정' }
    #  .env와 seccomp 프로필은 역할 폴더의 상위에서 공유한다.
    Remote $role "cd $($config.Root) && mv -f $role/.env .env && mv -f $role/seccomp-io-uring.json seccomp-io-uring.json && chmod 600 .env $role/*.env 2>/dev/null; true" 'transfer' | Out-Null
    Write-Host "transfer $role 완료"
}

function WaitUntil([string]$role, [string]$check, [int]$seconds, [string]$what) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $target = "$($config.User)@$($config.Hosts[$role])"
        & ssh -o BatchMode=yes -o ConnectTimeout=10 $target $check 2>&1 | Out-Null
        if ($LASTEXITCODE -eq 0) { return }
        Start-Sleep -Seconds 3
    }
    Fail 'start' $role "$seconds초 안에 $what 상태가 되지 않았다"
}

function StartRole([string]$role) {
    switch ($role) {
        'infra' {
            Compose $role 'up -d' 'start' | Out-Null
            WaitUntil $role "test `"`$(docker inspect --format '{{.State.Health.Status}}' portfolio26-infra-infra-1)`" = healthy" 600 'healthy'
        }
        'gameserver' {
            Compose $role 'up -d --force-recreate' 'start' | Out-Null
            WaitUntil $role "ss -ltnH 'sport = :$($config.GamePort)' | grep -q ." 120 "포트 $($config.GamePort) 수신"
        }
        'dummy' {
            #  이전 실행에서 로그인 상태로 남은 계정은 다시 로그인하지 못하므로 먼저 푼다.
            $reset = 'docker exec portfolio26-infra-infra-1 sh -c ''MYSQL_PWD="$MYSQL_ROOT_PASSWORD" mysql -uroot -e "update mydb.account set LoginState = 0 where LoginState = 1"'''
            $reset | & ssh -o BatchMode=yes "$($config.User)@$($config.Hosts.infra)" bash 2>&1 | Out-Null
            if ($LASTEXITCODE -ne 0) { Fail 'start' 'infra' '로그인 상태 초기화' }
            Compose $role 'up -d --force-recreate' 'start' | Out-Null
            WaitUntil $role "test `$(docker compose -p portfolio26-dummy ps -q --status running | wc -l) -eq 2" 60 '더미 2개 실행'
        }
    }
    Write-Host "start $role 완료"
}

function StopRole([string]$role) {
    Compose $role 'stop' 'stop' | Out-Null
    Write-Host "stop $role 완료"
}

function StatusRole {
    foreach ($r in @('infra', 'gameserver', 'dummy')) {
        Write-Host "== $r ($($config.Hosts[$r]))"
        $target = "$($config.User)@$($config.Hosts[$r])"
        & ssh -o BatchMode=yes -o ConnectTimeout=10 $target "docker ps -a --filter label=com.docker.compose.project=portfolio26-$r --format '{{.Names}}\t{{.Status}}\t{{.Image}}'"
        if ($LASTEXITCODE -ne 0) { Write-Host '  연결 실패' }
    }
}

switch ($Action) {
    'build' { foreach ($r in $roles) { BuildRole $r } }
    'transfer' { foreach ($r in $roles) { TransferRole $r } }
    'start' { foreach ($r in $roles) { StartRole $r } }
    'stop' {
        $order = if ($Role) { @($Role) } else { @('dummy', 'gameserver') }
        foreach ($r in $order) { StopRole $r }
    }
    'restart' {
        if (-not $Role) { throw 'restart는 -Role이 필요하다.' }
        StopRole $Role
        StartRole $Role
    }
    'status' { StatusRole }
    'deploy' {
        foreach ($r in $roles) { BuildRole $r }
        foreach ($r in $roles) { TransferRole $r }
        foreach ($r in $roles) { StartRole $r }
    }
}
