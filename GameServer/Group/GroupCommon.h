#pragma once
#include "Group.h"
#include "../GameMap.h"
#include "../Base/Player.h"

namespace psh
{
    class Server;
    
    class Player;
    class ChatCharacter;
    class GroupCommon : public Group
    {
    
    public:
        GroupCommon(Server* server, ServerType type,short mapSize = 6400, short sectorSize = 800):
        _server(server) ,_groupType(type), _playerMap(make_unique<GameMap<ChatCharacter>>(mapSize, sectorSize, server)),
        _prevUpdate(std::chrono::steady_clock::now()) {}
        
        void SetUseDB(const bool use){_useDB = use;}
        void OnUpdate() final;
        
        void OnEnter(SessionID id) override{}
        void OnLeave(SessionID id) override{}
        void OnRecv(SessionID id, CRecvBuffer& recvBuffer) override = 0;
        ~GroupCommon() override;
        virtual void CheckVictim(const Range& attackRange, int damage,const shared_ptr<ChatCharacter>& attacker){};
        virtual void CheckItem(const shared_ptr<ChatCharacter>& target){};
        

        void BroadcastMove(const shared_ptr<ChatCharacter>& player, FVector oldLocation, FVector newLocation);
        virtual void OnActorDestroy(const shared_ptr<GameObject>& object){}
        void SendCreate(const shared_ptr<GameObject>& object, FVector location, const std::span<const psh::Sector> offsets, bool isSpawn);
        void SendCreateAndGetInfo(const shared_ptr<psh::GameObject>& object, FVector location, const std::span<const psh::Sector> offsets, bool isSpawn);
        void SendDelete(const shared_ptr<psh::GameObject>& object, FVector location, const std::span<const psh::Sector> offsets, bool isDead);
        void SendDeleteAndGetInfo(const shared_ptr<psh::GameObject>& object, FVector location, const std::span<const psh::Sector> offsets, bool isDead);
        void Broadcast(FVector location,SendBuffer& buffer,GameObject* exclude = nullptr);

    protected:
        virtual void UpdateContent(float deltaMs) = 0;
        virtual void SendMonitor() = 0;

        Server* _server;
        //Info
        const ServerType _groupType = ServerType::End;
        unique_ptr<GameMap<ChatCharacter>> _playerMap;
        
    private:

        
       
        //DB
        bool _useDB = false;
        chrono::steady_clock::time_point _nextDBSend {};
        chrono::steady_clock::time_point _prevUpdate {};
        
        //Monitor
        long _groupSessionCount = 0;
        long _fps = 0;

        static SessionID _monitorSession;
        static String _monitorIp;
        static Port _monitorPort;

    };
    

}
