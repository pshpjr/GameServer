#pragma once

#include "Common/ContentTypes.h" //BYTE 종속성
// ReSharper disable once CppInconsistentNaming
enum en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE : BYTE;
class CLogger;

// 전방선언
namespace psh {
struct MonitorData;
class Item;
class Monster;
struct ServerInitData;
class DBThreadWrapper;
class GameObject;
class IVictimSelect;
class Player;
class Server;
class MonsterSpawner;
} // namespace psh

// 헤더 파일 포함

#include "GameMap.h"
#include "Group.h"
#include "IVictimSelect.h"
#include "Player.h"
#include <SPSCQueue.h>
#include <span>

// 네임스페이스 정의
namespace psh {
    //객체 생성, 삭제, 패킷 전송, 게임 업데이트, 주변 객체 정보 제공, DB에 로깅, 공격 처리, 세션 관리
    //TODO:하는 일이 너무 많은데
class Field final : public Group
{
    using MapType = GameMap<ObjectID, shared<GameObject>>;
    using ViewType = MapType::SectorView;

    struct PacketMonitor
    {
        int32 movePacket{};
        int32 chatPacket{};
        int32 attackPacket{};
    };

public:
    // 타입 정의 및 상수
    enum class ViewObjectType : uint8
    {
        Player = 1 << 1,
        Monster = 1 << 2,
        Item = 1 << 3,
        All = (1 << 1) | (1 << 2) | (1 << 3)
    };

    // 연산자 오버로딩
    friend ViewObjectType operator|(ViewObjectType lhs, ViewObjectType rhs);

    friend ViewObjectType operator&(ViewObjectType lhs, ViewObjectType rhs);

    // 생성자와 소멸자
    Field(Server& server, const ServerInitData& data, ServerType type, MonitorData& monitor,
          short map_size = 6400, short sector_size = 400);

    ~Field() override;

    // 공용 멤버 함수

    //부모 함수 오버라이드
    void OnEnter(SessionID id) override;

    void OnLeave(SessionID id, int wsaErrCode) override;


    void OnUpdate(int milli) override;

    void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;

    void OnCreate() override;

    //공격 처리
    AttackResult ProcessAttack(AttackInfo info);

    //주변 객체 관련
    ViewType GetObjectView(ViewObjectType type, const FVector& location,
                            std::span<const Sector> offsets);

    ViewType GetObjectViewByPoint(ViewObjectType type, const std::list<FVector>& coordinate);

    //객체 관리 부분
    void AddActor(const shared<GameObject>& obj);

    void DestroyActor(shared<GameObject>& obj);

    void DestroyActor(shared<GameObject>&& obj);

    void SpawnMonster(const shared<Monster>& obj);

    //패킷 전송 관련
    void BroadcastToPlayer(FVector targetLocation, const std::vector<SendBuffer>& packets);

    //세션 관리 부분
    void MoveField(SessionID id, GroupID gid);
private:
    // 비공용 멤버 함수
    [[nodiscard]] MapType* FindObjectMap(const shared<GameObject>& obj) const;

    //추가, 삭제 대기중인 객체 정리하는 함수들
    void InsertWaitObjectInMap();
    void CleanupDeleteWait();

    ObjectID NextObjectId();

    void UpdateContent(int deltaMs);

    void SendMonitor();

    //패킷 수신 관련 함수
    void RecvReqLevelChange(SessionID id, CRecvBuffer& recv_buffer);

    void RecvChangeComp(SessionID id, CRecvBuffer& recvBuffer);

    void RecvChat(SessionID id, CRecvBuffer& recvBuffer);

    void RecvMove(SessionID sessionId, CRecvBuffer& buffer);

    void RecvAttack(SessionID sessionId, CRecvBuffer& buffer);

    void SendLogin() const;

    //DB 콜백 처리
    void ProcessDatabaseAlerts();
    //DB 로깅
    void SendMonitorData(en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, int value) const;
    // 멤버 변수
    List<shared<GameObject>> _createWaits;
    List<shared<GameObject>> _delWaits;

    std::unique_ptr<SPSCQueue<std::function<void()>, 1024>> _dbCompAlert;
    std::unique_ptr<DBThreadWrapper> _dbThread;

    MonitorData& _monitorData;
    PacketMonitor _packetCount;
    Server& _server;
    const ServerInitData& _initData;
    const ServerType _groupType = ServerType::End;

    std::unordered_set<shared<GameObject>> _objects;
    SessionMap<std::shared_ptr<Player>> _players;

    //객체 타입별로 맵을 나눠서 탐색시 성능 올리기 위함
    std::shared_ptr<MapType> _playerMap;
    std::shared_ptr<MapType> _monsterMap;
    std::shared_ptr<MapType> _itemMap;
    victim_select::VictimSelectFunction _victimSelect;

    bool _useMonitor = false;
    std::chrono::steady_clock::time_point _nextDBSend{};
    std::chrono::steady_clock::time_point _prevUpdate{};

    std::unique_ptr<CLogger> _logger = nullptr;
    std::shared_ptr<MonsterSpawner> _spawner = nullptr;

    long _groupSessionCount = 0;
    long _fps = 0;

    SessionID _monitorSession = InvalidSessionID();
    ObjectID _objectId = 0;
        short _fieldSize = 0;
    };
}
