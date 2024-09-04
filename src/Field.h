#pragma once
#include "ContentTypes.h"
#include "Group.h"

#include "GameMap.h"


#include <range/v3/all.hpp>

enum en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE : BYTE;
class CLogger;

namespace psh
{
    struct ServerInitData;

    class DBThreadWrapper;
    class GameObject;
    class IVictimSelect;
    class Player;
    class Server;

    class Field final : public Group
    {
    public:
        Field(Server& server
            , const ServerInitData& data
            , ServerType type
            , short mapSize = 6400
            , short sectorSize = 400);

        ~Field() override;

        void OnEnter(SessionID id) override;
        void OnLeave(SessionID id, int wsaErrCode) override;
        void OnUpdate(int milli) override;
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override;
        void OnCreate() override;

        IVictimSelect* GetVictimSelect() const;

        decltype(auto) GetPlayerView(const FVector& location, const std::span<const Sector> offsets)
        {
            return _playerMap->GetSectorsFromOffset(location, offsets) | ranges::to<std::vector>;
        }

        decltype(auto) GetPlayerView(const Sector& sector, const std::span<const Sector> offsets)
        {
            return _playerMap->GetSectorsFromOffset(sector, offsets) | ranges::to<std::vector>;
        }

        decltype(auto) GetObjectView(const FVector& location, const std::span<const Sector> offsets)
        {

            return ranges::views::concat(_playerMap->GetSectorsFromOffset(location, offsets)
                , _monsterMap->GetSectorsFromOffset(location, offsets)
                , _itemMap->GetSectorsFromOffset(location, offsets));
        }

        decltype(auto) GetMonsterView(const FVector& location, const std::span<const Sector> offsets)
        {
            return _monsterMap->GetSectorsFromOffset(location, offsets);
        }

        decltype(auto) GetItemView(const FVector& location, const std::span<const Sector> offsets)
        {
            return _itemMap->GetSectorsFromOffset(location, offsets);
        }

        decltype(auto) GetItemViewByList(const std::list<FVector>& locations)
        {
            return _itemMap->GetSectorsByList(locations);
        }


        void AddActor(const shared<GameObject>& obj);
        void DestroyActor(shared<GameObject>& obj);
        void DestroyActor(shared<GameObject>&& obj);

    protected:
        List<shared<GameObject>> _createWaits;
        List<shared<GameObject>> _delWaits;

        [[nodiscard]] GameMap<shared<GameObject>>* FindObjectMap(const shared<GameObject>& obj) const;
        void InsertWaitObjectInMap();
        void CleanupDeleteWait();
        ObjectID NextObjectId();
        void UpdateContent(int deltaMs);
        void SendMonitor();
        std::unique_ptr<DBThreadWrapper> _dbThread;

        //Info
        Server& _server;
        const ServerInitData& _initData;
        const ServerType _groupType = ServerType::End;

        using map_type = GameMap<shared<GameObject>>;

        std::unordered_set<shared<GameObject>> _objects;

        SessionMap<std::shared_ptr<Player>> _players;
        std::shared_ptr<map_type> _playerMap;
        std::shared_ptr<map_type> _monsterMap;
        std::shared_ptr<map_type> _itemMap;
        std::unique_ptr<IVictimSelect> _victimSelect;
    private:
        void RecvReqLevelChange(SessionID id, CRecvBuffer& recvBuffer);
        void RecvChangeComp(SessionID id, CRecvBuffer& recvBuffer);
        void RecvChat(SessionID id, CRecvBuffer& recvByffer);
        void RecvMove(SessionID sessionId, CRecvBuffer& buffer);
        void RecvAttack(SessionID sessionId, CRecvBuffer& buffer);

        //DB
        bool _useMonitor = false;
        std::chrono::steady_clock::time_point _nextDBSend{};
        std::chrono::steady_clock::time_point _prevUpdate{};

        //Monitor
        void SendLogin() const;
        void SendMonitorData(en_PACKET_SS_MONITOR_DATA_UPDATE_TYPE type, int value) const;
        std::unique_ptr<CLogger> _logger = nullptr;


        long _groupSessionCount = 0;
        long _fps = 0;

        SessionID _monitorSession = InvalidSessionID();
        ObjectID _objectId = 0;
        int _fieldSize = 0;
    };
}
