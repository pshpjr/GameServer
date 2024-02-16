#include "stdafx.h"
#include "GameMap.h"

#include <functional>
#include <PacketGenerated.h>

#include "Profiler.h"
#include "Sector.h"
#include "ContentTypes.h"
#include "Player.h"
#include "Server.h"
#include <iostream>

//특정 방향으로 이동할 때 탐색해야 할 섹터. 방향마다 크기가 다름. 
vector<vector<vector<pair<short,short>>>> AddTable =
{
    {
        {{-1, -1}, {-1, 0}, {-1, 1}, {1, -1}, {0, -1}}, {{-1, -1}, {-1, 0}, {-1, 1}}, {{-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}}
    },
    {
		    {{1, -1}, {0, -1}, {-1, -1}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{-1, 1}, {0, 1}, {1, 1}}
    },
  {
        {{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}}, {{1, 1}, {1, 0}, {1, -1}}, {{-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}}
    }
};

vector<vector<vector<pair<short,short>>>> DeleteTable =
{
    {
        {{-1, 1}, {0, 1}, {1, 1}, {1, 0}, {1, -1}}, {{1, 1}, {1, 0}, {1, -1}}, {{1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}}
    },
    {
		     {{-1, 1}, {0, 1}, {1, 1}}, {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}, {{1, -1}, {0, -1}, {-1, -1}}
    },
    {
		    {{-1, -1}, {-1, 0}, {-1, 1}, {0, 1}, {1, 1}}, {{-1, -1}, {-1, 0}, {-1, 1}}, {{-1, -1}, {-1, 0}, {-1, 1}, {1, -1}, {0, -1}}
    }
};

static constexpr std::array<pair<short,short>,9> broadcast = {{
    {-1, 0}, {0, -1}, {1, 0}
    ,{0,0}, {0, 1}, {-1, -1}
    , {1, -1}, {-1, 1}, {1, 1}}};

psh::GameMap::GameMap(const short mapSize, const short sectorSize, Server* owner,ServerType type)
    :SECTOR_SIZE(sectorSize),MAP_SIZE(mapSize)
,MAX_SECTOR_INDEX_X(MAP_SIZE / SECTOR_SIZE - 1),
MAX_SECTOR_INDEX_Y(MAP_SIZE / SECTOR_SIZE - 1), _owner(owner)
{
    ASSERT_CRASH(SECTOR_SIZE > 0,"Invalid Sector Size");
    _map.resize(MAP_SIZE / SECTOR_SIZE);
    for(auto& i :_map)
    {
        i.resize(MAP_SIZE / SECTOR_SIZE);
    }
}

auto psh::GameMap::GetSectorsViewFromOffset(const Sector& targetSector, std::span<const pair<short,short>> offsets)
{
    //오프셋을 섹터로 변환.
    auto sectors= std::views::all(offsets)
        | std::views::transform([&](const auto offset){ 
               short x = targetSector.x + offset.first;
               short y = targetSector.y + offset.second;
               return Sector{x, y}; });

    //가능한 섹터만 필터링. 
    auto valid = sectors|std::views::filter([&](const auto sector)
    {
        if(IsValidSector(sector))
        {
            return true;
        }
        return false;
    });

    //섹터를 맵의 섹터 포인터로 변환
    auto refs = valid | std::views::transform([&](const auto sector){ 
               return std::ref(_map[sector.y][sector.x]); });
    return refs;
}

void psh::GameMap::PrintPlayerInfo()
{
    auto tmp = GetPlayerInfo();
    
    for(auto& col : tmp)
    {
        for(auto& sector : col)
        {
            cout << sector <<' ';
        }
        cout <<'\n';
    }

}

vector<vector<int>> psh::GameMap::GetPlayerInfo()
{
    vector<vector<int>> ret(MAX_SECTOR_INDEX_Y,vector<int>(MAX_SECTOR_INDEX_X));

    for(int i =0; i<MAX_SECTOR_INDEX_Y; i++)
    {
        for(int j = 0;j< MAX_SECTOR_INDEX_X; j++)
        {
            ret[MAX_SECTOR_INDEX_X - j - 1][ i] = _map[i][j].size();
        }
    }
    return ret;
}

std::span<const pair<short, short>> psh::GameMap::GetMoveOffset(Sector moveInfo)
{
    
}

//새로 플레이어 생성하는 함수. 주변에 뿌린다. 
void psh::GameMap::SpawnPlayer(shared_ptr<Player>& target, psh::ServerType type)
{
    auto curSector = GetSector(target->Location());
    //추가하고
    AddPlayer(curSector,target);
    

    //알린다. 
    SendCreateCharacter(target,true, broadcast);
}

void psh::GameMap::AddPlayer(shared_ptr<Player>& target)
{
    const auto targetSector = GetSector(target->Location());

    ASSERT_CRASH(0 <= targetSector.x && targetSector.x <=MAX_SECTOR_INDEX_X,"Invalid xLocation");
    ASSERT_CRASH(0 <= targetSector.y && targetSector.x <=MAX_SECTOR_INDEX_Y,"Invalid yLocation");
    
    AddPlayer(targetSector,target);
}

void psh::GameMap::AddPlayer(Sector targetSector, shared_ptr<Player>& target)
{
    //_map[targetSector.y][targetSector.x].push_back(target);
    _map[targetSector.y][targetSector.x].insert(target);
}

void psh::GameMap::SendCreateCharacter(shared_ptr<Player>& target, bool isSpawn, std::span<const pair<short, short>> offsets)
{
    auto myInfoBuffer =  SendBuffer::Alloc();
    MakeGame_ResCreateActor(myInfoBuffer,target->AccountNumber(),isSpawn,Player1,target->Location(),target->Direction());
    
    const auto targetSector = GetSector(target->Location());
    auto addSectors = GetSectorsViewFromOffset(targetSector,broadcast);

    //정보를 받아옴. 
    auto otherInfoBuffer =SendBuffer::Alloc();
    ranges::for_each(addSectors,[&myInfoBuffer,&otherInfoBuffer, this,&target](playerSecor& sector)
    {
        for(std::shared_ptr<psh::Player>& player : sector)
        {
            if (player != target)
            {
                printf(format("Send create {:d} to {:d}  {:f} {:f} {:f}\n",target->SessionId().id,player->SessionId().id, target->Location().X,target->Location().Y,target->Location().Z).c_str());
                _owner->SendPacket(player->SessionId(), myInfoBuffer);
                printf(format("Add otherPlayer {:d} to {:d}|  {:f} {:f} {:f} \n",player->SessionId().id, target->SessionId().id, player->Location().X, player->Location().Y, player->Location().Z).c_str());
                MakeGame_ResCreateActor(otherInfoBuffer, player->AccountNumber(), false, Player1, player->Location(), player->Direction());
            }
        }
    });

    //각각 전송. 
    printf(format("Create You. you :  {:d} \n",target->SessionId().id).c_str());
    _owner->SendPacket(target->SessionId(), myInfoBuffer);
    if(otherInfoBuffer.Size() != 0)
    {
        printf(format("Send otherPlayer Info {:d} \n",target->SessionId().id).c_str());
        _owner->SendPacket(target->SessionId(), otherInfoBuffer);
    }
}

//플레이어 제거하라고 알리는 함수.
void psh::GameMap::SendRemoveCharacter(shared_ptr<Player>& target, bool isDead, std::span<const pair<short, short>> offsets)
{
    auto destroyThisBuffer =  SendBuffer::Alloc();
    MakeGame_ResDestroyActor(destroyThisBuffer,target->AccountNumber(),isDead);
    
    const auto targetSector = GetSector(target->Location());

    SendToSectors(targetSector,destroyThisBuffer,offsets);
}
//공격했다는 걸 알리는 함수.
void psh::GameMap::BroadcastAttack(shared_ptr<Player>& target)
{
    auto attackPacket =  SendBuffer::Alloc();
    MakeGame_ResAttack(attackPacket,target->AccountNumber());

    const auto targetSector = GetSector(target->Location());

    SendToSectors(targetSector,attackPacket, broadcast);
}

void psh::GameMap::BroadcastHit(shared_ptr<Player>& target)
{
    
}

//이동 시작하는 걸 알리는 함수. 
void psh::GameMap::BroadcastMoveStart(shared_ptr<Player>& target, const FVector newLocation)
{
    printf("Move %lld to %f %f %f  \n "
       "cur : %f %f %f \n",target->SessionId().id, newLocation.X, newLocation.Y, newLocation.Z
       ,target->Location().X,target->Location().Y,target->Location().Z);
    
    auto curSector = GetSector(target->Location()); 
    
    auto playerMoveBuffer = SendBuffer::Alloc();
    MakeGame_ResMove(playerMoveBuffer,target->AccountNumber(),newLocation);

    SendToSectors(curSector,playerMoveBuffer, broadcast);
}

void psh::GameMap::SendToSectors(const Sector& targetSector, SendBuffer& buffer, std::span<const pair<short, short>> offsets)
{
    auto broadcastSectors = GetSectorsViewFromOffset(targetSector,broadcast);
    ranges::for_each(broadcastSectors,[ this,&buffer](playerSecor& sector)
    {
        for(auto& player : sector)
        {
            _owner->SendPacket(player->SessionId(), buffer,0);
        }
    });
}

//플레이어를 실제로 이동시키는 함수.
void psh::GameMap::MovePlayer(shared_ptr<Player>& target, const FVector newLocation)
{
    auto curSector = GetSector(target->Location());
    auto newSector = GetSector(newLocation);
    //플레이어를 이동한다.     
    target->Location(newLocation);
    
    //섹터가 변경되었으면
    if(curSector == newSector)
    {
        return;
    }

    //이전 섹터에서 지우고
    RemovePlayer(curSector,target);
    //삭제해야 할 섹터들에 지우라고 알리고

    //다음 섹터에 추가하고
    AddPlayer(newSector,target);
    //생성하라고 알리기. (스폰 아님)
}

//있는 플레이어 삭제하는 함수. 
void psh::GameMap::DestroyPlayer(shared_ptr<Player>& target)
{
    auto curSector = GetSector(target->Location());
    //삭제하고 
    RemovePlayer(curSector,target);
    //알린다. 
    SendRemoveCharacter(target,false, broadcast);
}

psh::Sector psh::GameMap::GetSector(const FVector location) const
{
    //입력이 비정상이면 비정상적인 값 줌.
    //여기가 모든 입력을 걸러준다는 가정. 
    if (location.X < 0 || MAP_SIZE <= location.X  || location.Y < 0 || MAP_SIZE <= location.Y )
        return {-1,-1};
    
    return {static_cast<short>(location.X / SECTOR_SIZE), static_cast<short>(location.Y / SECTOR_SIZE)};
}

bool psh::GameMap::IsValidSector(const Sector sector) const
{
    if (sector.x < 0 || sector.y < 0)
        return false;

    if (MAX_SECTOR_INDEX_X < sector.x || MAX_SECTOR_INDEX_Y < sector.y)
        return false;

    return true;
}

void psh::GameMap::UpdatePlayer()
{
    

    
}

void psh::GameMap::RemovePlayer(shared_ptr<Player>& target)
{
    const auto removeSector = GetSector(target->Location());

    RemovePlayer(removeSector,target);
}

void psh::GameMap::RemovePlayer(Sector curSector, shared_ptr<Player>& target)
{
    //_map[curSector.y][curSector.x].erase(remove(_map[curSector.y][curSector.x].begin(),_map[curSector.y][curSector.x].end(), target),_map[curSector.y][curSector.x].end());
   _map[curSector.y][curSector.x].erase(target);
}
        