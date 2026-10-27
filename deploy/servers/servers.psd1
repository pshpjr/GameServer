# 서버 배포 설정. deploy.ps1이 읽는다.
@{
    User = 'pshpjr'
    # 세 서버 공통 작업 디렉터리
    Root = '/home/pshpjr/portfolio26'
    Hosts = @{
        infra      = '192.168.1.1'
        gameserver = '192.168.1.5'
        dummy      = '192.168.1.3'
    }
    # 빌드 설정. linux-Release 프리셋은 RelWithDebInfo다.
    BuildPreset = 'linux-Release'
    EnableOtel = 'ON'
    EnableTracy = 'OFF'
    VcpkgCacheImage = 'ghcr.io/pshpjr/gameserver:vcpkg-cache'
    # 이미지 태그. build가 덮어쓰고, transfer는 이 태그를 보낸다.
    Images = @{
        infra      = 'portfolio26-infra:server'
        gameserver = 'portfolio26-gameserver:server'
        dummy      = 'portfolio26-dummy:server'
    }
    GamePort = 11333
}
