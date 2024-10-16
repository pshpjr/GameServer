#pragma once

// 전방 선언(forward declarations)
#include "ContentTypes.h"//BYTE 종속성
enum en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE : BYTE;
class CLogger;

namespace psh
{
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
}

// 헤더 파일 포함

#include "Group.h"
#include "GameMap.h"
#include "IVictimSelect.h"
#include "Player.h"

#include <range/v3/all.hpp>

// 네임스페이스 정의
namespace psh
{
    class Field final : public Group
    {
        using map_type = GameMap<ObjectID, shared<GameObject>>;
        using view_type = map_type::SectorView;

        struct packetMonitor
        {
            int32 movePacket{};
            int32 chatPacket{};
            int32 attackPacket{};
        };

    public:
        // 타입 정의 및 상수
        enum class ViewObjectType : uint8
        {
            Player = 1 << 1, Monster = 1 << 2, Item = 1 << 3, All = (1 << 1) | (1 << 2) | (1 << 3)
        };

        // 연산자 오버로딩
        friend ViewObjectType operator|(ViewObjectType lhs, ViewObjectType rhs);

        friend ViewObjectType operator&(ViewObjectType lhs, ViewObjectType rhs);

        // 생성자와 소멸자
        Field(Server& server, const ServerInitData& data, ServerType type, MonitorData& monitor, short mapSize = 6400
              , short sectorSize = 400);

        ~Field() override;

        // 공용 멤버 함수
        void OnEnter(SessionID id) override;

        void OnLeave(SessionID id, int wsaErrCode) override;
        void ProcessDatabaseAlerts();

        void OnUpdate(int milli) override;

        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;

        void OnCreate() override;

        AttackResult ProcessAttack(AttackInfo info);

        view_type GetObjectView(ViewObjectType type, const FVector& location, std::span<const Sector> offsets);

        view_type GetObjectViewByPoint(ViewObjectType type, const std::list<FVector>& coordinate);

        void AddActor(const shared<GameObject>& obj);

        void DestroyActor(shared<GameObject>& obj);

        void DestroyActor(shared<GameObject>&& obj);

        void BroadcastToPlayer(FVector targetLocation, const std::vector<SendBuffer>& packets);

        void SpawnMonster(const shared<Monster>& obj);

        void MoveField(SessionID id, GroupID gid);

    private:
        // 비공용 멤버 함수
        [[nodiscard]] map_type* FindObjectMap(const shared<GameObject>& obj) const;

        void InsertWaitObjectInMap();

        void CleanupDeleteWait();

        ObjectID NextObjectId();

        void UpdateContent(int deltaMs);

        void SendMonitor();

        void RecvReqLevelChange(SessionID id, CRecvBuffer& recvBuffer);

        void RecvChangeComp(SessionID id, CRecvBuffer& recvBuffer);

        void RecvChat(SessionID id, CRecvBuffer& recvByffer);

        void RecvMove(SessionID sessionId, CRecvBuffer& buffer);

        void RecvAttack(SessionID sessionId, CRecvBuffer& buffer);

        void SendLogin() const;

        void SendMonitorData(en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, int value) const;
        // 멤버 변수
        List<shared<GameObject>> _createWaits;
        List<shared<GameObject>> _delWaits;

        std::unique_ptr<LockFreeFixedQueue<std::function<void()>, 1024>> _dbCompAlert;
        std::unique_ptr<DBThreadWrapper> _dbThread;

        MonitorData& _monitorData;
        packetMonitor _packetCount;
        Server& _server;
        const ServerInitData& _initData;
        const ServerType _groupType = ServerType::End;


        std::unordered_set<shared<GameObject>> _objects;
        SessionMap<std::shared_ptr<Player>> _players;
        std::shared_ptr<map_type> _playerMap;
        std::shared_ptr<map_type> _monsterMap;
        std::shared_ptr<map_type> _itemMap;
        victim_select::VictimSelectFunction _victimSelect;

        bool _useMonitor = false;
        std::chrono::steady_clock::time_point _nextDBSend{};
        std::chrono::steady_clock::time_point _prevUpdate{};

        std::unique_ptr<CLogger> _logger = nullptr;
        std::unique_ptr<MonsterSpawner> _spawner = nullptr;

        long _groupSessionCount = 0;
        long _fps = 0;

        SessionID _monitorSession = InvalidSessionID();
        ObjectID _objectId = 0;
        short _fieldSize = 0;
    };
}
