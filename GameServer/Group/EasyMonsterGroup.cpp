#include "EasyMonsterGroup.h"

#include <PacketGenerated.h>

#include "../Base/ObjectManager.h"
#include "FieldObjectManager.h"
#include "PveAttackManager.h"
#include "IOCP.h"
#include "Profiler.h"

namespace psh
{
    EasyMonsterGroup::EasyMonsterGroup(Server& server,ServerType type, short mapSize, short sectorSize):
        GroupCommon(server
        , type,
        mapSize, sectorSize)
        , _monsterMap(make_unique<GameMap<shared_ptr<Monster>>>(mapSize, sectorSize))
        , _itemMap(make_unique<GameMap<shared_ptr<Item>>>(mapSize, sectorSize))
    {
        _objectManager = 
        unique_ptr<ObjectManager>{
            reinterpret_cast<psh::ObjectManager*>(
                new FieldObjectManager(*this,*_playerMap,*_monsterMap,*_itemMap))},
        _attackManager = static_cast<unique_ptr<AttackManager>>(make_unique<PveAttackManager>(*_monsterMap,*_playerMap));
    }

    void EasyMonsterGroup::OnRecv(const SessionID id, CRecvBuffer& recvBuffer)
    {
        ePacketType type;
        recvBuffer >> type;
        switch (type)
        {
            case eGame_ReqChangeComplete:
                RecvChangeComp(id, recvBuffer);
                break;
            case eGame_ReqMove:
                RecvMove(id, recvBuffer);
                break;
            case eGame_ReqAttack:
                RecvAttack(id, recvBuffer);
                break;
            case eGame_ReqLevelEnter:
                RecvReqLevelChange(id, recvBuffer);
                break;
            default:
                DebugBreak();
                break;
        }
    }
    

    void EasyMonsterGroup::SendMonitor()
    {
        
    }
    


    EasyMonsterGroup::~EasyMonsterGroup() = default;


    


}
