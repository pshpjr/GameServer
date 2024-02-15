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


static constexpr std::array<pair<short,short>,9> broadcast = {{
    {-1, 0}, {0, -1}, {1, 0}
    ,{0,0}, {0, 1}, {-1, -1}
    , {1, -1}, {-1, 1}, {1, 1}}};

psh::GameMap::GameMap(const int mapSize, const int sectorSize, Server* owner)
    :SECTOR_SIZE(sectorSize),MAP_SIZE(mapSize)
,MAX_SECTOR_INDEX_X(MAP_SIZE / SECTOR_SIZE -1),
MAX_SECTOR_INDEX_Y(MAP_SIZE / SECTOR_SIZE -1), _owner(owner)
{
    
    ASSERT_CRASH(SECTOR_SIZE > 0,"Invalid Sector Size");
    _map.resize(MAP_SIZE / SECTOR_SIZE);
    for(auto& i :_map)
    {
        i.resize(MAP_SIZE / SECTOR_SIZE);
    }
    
   // _map(MAP_SIZE / SECTOR_SIZE, ::vector<LockSector>(MAP_SIZE / SECTOR_SIZE));
}
auto psh::GameMap::GetSectorsViewFromOffset(const Sector& targetSector, std::span<const pair<short,short>> offsets)
{
    auto sectors= std::views::all(offsets)
        | std::views::transform([&](const auto offset){ 
               short x = targetSector.x + offset.first;
               short y = targetSector.y + offset.second;
               return Sector{x, y}; });
    
    auto valid = sectors|std::views::filter([&](const auto sector)
    {
        if(IsValidSector(sector))
        {
            return true;
        }
        return false;
    });
    
    auto refs = valid | std::views::transform([&](const auto sector){ 
               return std::ref(_map[sector.y][sector.x]); });
    return refs;
}

void psh::GameMap::PrintPlayerInfo()
{
    // auto targetSector = GetSector({300,300});
    //
    // auto view = GetSectorsViewFromOffset(targetSector,broadcast);
    // ranges::for_each(view,[](LockSector& sector)
    // {
    //     printf("%d ", sector.size());
    // });
    // printf("\n");
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

void psh::GameMap::AddPlayer(shared_ptr<Player>& target)
{
    const auto targetSector = GetSector(target->Location());

    ASSERT_CRASH(0 <= targetSector.x && targetSector.x <=MAX_SECTOR_INDEX_X,"Invalid xLocation");
    ASSERT_CRASH(0 <= targetSector.y && targetSector.x <=MAX_SECTOR_INDEX_Y,"Invalid yLocation");
    
    AddPlayer(targetSector,target);
    
}

void psh::GameMap::BroadcastNewPlayerCreated(shared_ptr<Player>& target)
{
    auto myInfoBuffer =  SendBuffer::Alloc();
    MakeGame_ResCreateUser(myInfoBuffer,target->AccountNumber(),1,1,target->Location(),target->Direction());

    auto otherInfoBuffer =SendBuffer::Alloc();

    const auto targetSector = GetSector(target->Location());

    auto addSectors = GetSectorsViewFromOffset(targetSector,broadcast);
    
    ranges::for_each(addSectors,[&myInfoBuffer,&otherInfoBuffer, this,&target](LockSector& sector)
    {
        for(std::shared_ptr<psh::Player>& player : sector)
        {
            if (player != target)
            {
                printf(format("Send create {:d} to {:d}  {:f} {:f} {:f}\n",target->SessionId().id,player->SessionId().id, target->Location().X,target->Location().Y,target->Location().Z).c_str());
                _owner->SendPacket(player->SessionId(), myInfoBuffer);
                printf(format("Add otherPlayer {:d} to {:d}|  {:f} {:f} {:f} \n",player->SessionId().id, target->SessionId().id, player->Location().X, player->Location().Y, player->Location().Z).c_str());
                MakeGame_ResCreateUser(otherInfoBuffer, player->AccountNumber(), 1, 1, player->Location(), player->Direction());
            }
        }
    });
    printf(format("Create You. you :  {:d} \n",target->SessionId().id).c_str());
    _owner->SendPacket(target->SessionId(), myInfoBuffer);
    if(otherInfoBuffer.Size() != 0)
    {
        printf(format("Send otherPlayer Info {:d} \n",target->SessionId().id).c_str());
        _owner->SendPacket(target->SessionId(), otherInfoBuffer);
    }

}

void psh::GameMap::BroadcastAttack(shared_ptr<Player>& target)
{
    auto attackPacket =  SendBuffer::Alloc();
    MakeGame_ResAttack(attackPacket,target->AccountNumber());

    const auto targetSector = GetSector(target->Location());
    auto addSectors = GetSectorsViewFromOffset(targetSector,broadcast);
    
    ranges::for_each(addSectors,[&attackPacket, this](LockSector& sector)
    {
        for(std::shared_ptr<psh::Player>& player : sector)
        {
            _owner->SendPacket(player->SessionId(), attackPacket);
        }
    });
}

/*
 * 좌표 범위에 있는 플레이어들의 리스트를 반환한다.
 * 이 때 잘못된 범위는 제외한다.
 * 
 */



void psh::GameMap::RemovePlayer(shared_ptr<Player>& target)
{
    const auto removeSector = GetSector(target->Location());

    RemovePlayer(removeSector,target);
}



void psh::GameMap::MovePlayer(shared_ptr<Player>& target, const FVector newLocation)
{

    printf("Move %lld to %f %f %f  \n "
           "cur : %f %f %f \n",target->SessionId().id, newLocation.X, newLocation.Y, newLocation.Z
           ,target->Location().X,target->Location().Y,target->Location().Z);
    
    auto curSector = GetSector(target->Location());
    auto newSector = GetSector(newLocation);
    
    auto playerMoveBuffer = SendBuffer::Alloc();
    MakeGame_ResMove(playerMoveBuffer,target->AccountNumber(),newLocation);
    
    
    auto broadcastSectors = GetSectorsViewFromOffset(curSector,broadcast);
    ranges::for_each(broadcastSectors,[ this,&playerMoveBuffer,&target](LockSector& sector)
    {
        for(auto& player : sector)
        {
            _owner->SendPacket(player->SessionId(), playerMoveBuffer,0);
        }
    });
    target->Location(newLocation);
    
    
    if(curSector == newSector)
    {
        return;
    }
    
    RemovePlayer(curSector,target);
    AddPlayer(newSector,target);
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

void psh::GameMap::MovePlayer(psh::FVector oldLocation, psh::FVector newLocation, shared_ptr<Player>& target)
{
    const auto oldSector = GetSector(oldLocation);
    const auto newSector = GetSector(newLocation);

    if(oldSector == newSector)
        return;


    {
        PRO_BEGIN("Remove")
        RemovePlayer(oldSector,target);
    }

    {
        PRO_BEGIN("Add")
        AddPlayer(newSector,target);
    }


}

void psh::GameMap::Iterate(FVector radius, const function<void(Player&)>& toInvoke)
{
}


void psh::GameMap::ApplyToPlayerInSector(Sector target, const std::function<void(psh::Player&)>& toInvoke)
{
    for(auto& player: _map[target.y][target.x])
    {
        toInvoke(*player.get());
    }
}

void psh::GameMap::AcquireExclusiveLock(const Sector targetSector)
{
    _map[targetSector.y][targetSector.x].lock->lock();
}

void psh::GameMap::ReleaseExclusiveLock(const Sector targetSector)
{
    _map[targetSector.y][targetSector.x].lock->unlock();
}

void psh::GameMap::AcquireSharedLock(const Sector targetSector)
{
    _map[targetSector.y][targetSector.x].lock->lock_shared();
}

void psh::GameMap::ReleaseSharedLock(const Sector targetSector)
{
    _map[targetSector.y][targetSector.x].lock->unlock_shared();
}


void psh::GameMap::Iterate(psh::FVector start, psh::FVector end, const function<void(Player&)>& toInvoke)
{
    //start < end 형태가 되게 변환한다.

    if(start > end)
        ::swap(start,end);
    
    //가능한 범위로 수정한다.
    const auto startPoint = psh::Ceil(start,0);
    const auto endPoint = psh::Floor(end,MAP_SIZE-1);


    //시작 섹터와 끝 섹터를 구한다.
    const auto startSector = GetSector(startPoint);
    const auto endSector = GetSector(endPoint);

    for(short y = startSector.y; y<= endSector.y; ++y)
    {
        for(short x = startSector.x; x<= endSector.x; ++x)
        {
            ApplyToPlayerInSector({x,y}, toInvoke);
        }
    };
}

void psh::GameMap::AddPlayer(Sector targetSector, shared_ptr<Player>& target)
{
   //_map[targetSector.y][targetSector.x].push_back(target);
    _map[targetSector.y][targetSector.x].insert(target);

}

void psh::GameMap::RemovePlayer(Sector curSector, shared_ptr<Player>& target)
{
    //_map[curSector.y][curSector.x].erase(remove(_map[curSector.y][curSector.x].begin(),_map[curSector.y][curSector.x].end(), target),_map[curSector.y][curSector.x].end());
   _map[curSector.y][curSector.x].erase(target);
}
        