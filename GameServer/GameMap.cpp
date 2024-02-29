#include "stdafx.h"
#include "GameMap.h"

#include <functional>
#include <PacketGenerated.h>

#include "Profiler.h"
#include "Sector.h"
#include "ContentTypes.h"
#include "Server.h"
#include "Data/TableData.h"
#include <iostream>

#include "Base/Player.h"


//가로가 x, 세로가 y.
//y를 가지고 있는 x벡터들
psh::GameMap::GameMap(const short mapSize, const short sectorSize, Server* owner)
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

    _grid.resize(MAP_SIZE/GRID_SIZE);
    for(auto& i :_map)
    {
        i.resize(MAP_SIZE/GRID_SIZE);
    }
}

auto psh::GameMap::GetSectorsFromOffset(const psh::Sector& target, std::span<const psh::Sector> offsets) const 
{
    // Convert offset to sectors
    return std::views::all(offsets)
        | std::views::transform([&](const Sector offset){
            const short x = target.x + offset.x;
            const short y = target.y + offset.y;
            return psh::Sector{x, y}; });
}

template<class T>
auto FlatRange(T xBegin, T xEnd, T yBegin, T yEnd)
{
    return std::views::iota(xBegin, xEnd+1)
    | std::views::transform([=](auto x) 
        { 
            return std::views::iota(yBegin, yEnd+1) 
            | std::views::transform([=](auto y) 
                { 
                    return psh::Sector{x, y}; 
                }); 
        })
    | std::views::join;
}

auto psh::GameMap::GetSectorsFromRange(const FVector& point1, const FVector& point2) const
{
    const auto p1Sector = GetSector(point1);
    const auto p2Sector = GetSector(point2);

    return FlatRange(p1Sector.x, p2Sector.x, p1Sector.y, p2Sector.y);
}

auto psh::GameMap::GetValidSectorView()
{
    return std::views::filter([this]( Sector sector)
            {
                return IsValidSector(sector);
            })
            | std::views::transform([this](Sector sector)
            {
                return std::ref(_map[sector.x][sector.y]);
            });
}


auto psh::GameMap::GetSituatedSectorView(const psh::Sector& target, const std::span<const psh::Sector> offsets)
{
    auto sectors = GetSectorsFromOffset(target, offsets);
    return sectors | GetValidSectorView();
}

auto psh::GameMap::GetVicinalSectorView(const FVector& p1, const FVector& p2)
{
    auto sectors = GetSectorsFromRange(p1, p2);
    return sectors | GetValidSectorView();
}

void psh::GameMap::PrintPlayerInfo()
{
    const auto tmp = GetPlayerInfo();
    
    for(auto& col : tmp)
    {
        for(const auto& sector : col)
        {
            cout << sector <<' ';
        }
        cout <<'\n';
    }

}


vector<vector<int>> psh::GameMap::GetPlayerInfo()
{
    vector ret(MAX_SECTOR_INDEX_Y,vector<int>(MAX_SECTOR_INDEX_X));

    for(int i =0; i<MAX_SECTOR_INDEX_X; i++)
    {
        for(int j = 0;j< MAX_SECTOR_INDEX_Y; j++)
        {
            ret[MAX_SECTOR_INDEX_X - i - 1][ j] = _map[i][j].size();
        }
    }
    return ret;
}

int psh::GameMap::Players()
{
    int count = 0;
    for (auto& col : _map)
    {
        for (auto& row : col)
        {
            count += row.size();
        }
    }
    return count;
}

//새로 플레이어 생성하는 함수. 주변에 뿌린다. 
void psh::GameMap::SpawnPlayer(shared_ptr<Player>& target)
{
    const auto curSector = GetSector(target->Location());
    
    //알리고 
    SendCreateCharacterAndGetInfo(target);

    //추가한다. 
    AddPlayer(curSector, target);
}

void psh::GameMap::AddPlayer(const shared_ptr<psh::Player>& target)
{
    const auto targetSector = GetSector(target->Location());

    ASSERT_CRASH(0 <= targetSector.x && targetSector.x <=MAX_SECTOR_INDEX_X,"Invalid xLocation");
    ASSERT_CRASH(0 <= targetSector.y && targetSector.x <=MAX_SECTOR_INDEX_Y,"Invalid yLocation");
    
    AddPlayer(targetSector,target);
}

void psh::GameMap::AddPlayer(const Sector targetSector, const shared_ptr<psh::Player>& target)
{
    //_map[targetSector.y][targetSector.x].push_back(target);
    _map[targetSector.x][targetSector.y].insert(target);
}

void psh::GameMap::SendCreateCharacterAndGetInfo(const shared_ptr<psh::Player>& target)
{
    auto myInfoBuffer =  SendBuffer::Alloc();
    target->GetInfo(myInfoBuffer,true);
    
    const auto targetSector = GetSector(target->Location());
    
    auto addSectors = GetSituatedSectorView(targetSector,BROADCAST);
    
    //정보를 받아옴.
    vector<SendBuffer> toSend;
    toSend.push_back(SendBuffer::Alloc());
    
    ranges::for_each(addSectors,[&myInfoBuffer,&toSend, this,&target](playerSector& sector)
    {
        for(std::shared_ptr<psh::Player>& player : sector)
        {
            if (player == target)
            {
                continue;
            }
            
            _owner->SendPacket(player->SessionId(), myInfoBuffer);
    
            if(toSend.back().CanPushSize() < 100)
            {
                toSend.push_back(SendBuffer::Alloc());
            }
            player->GetInfo(toSend.back(),false);
        }
    });
    
    //각각 전송. 
    //printf(format("Create You. you :  {:d} \n",target->SessionId().id).c_str());
    _owner->SendPacket(target->SessionId(), myInfoBuffer);
    
    if(toSend.back().Size() != 0)
    {
        _owner->SendPackets(target->SessionId(), toSend);
    }
}



//공격했다는 걸 알리는 함수.
void psh::GameMap::CheckVictim(const psh::Range& attackRange, int damage,const shared_ptr<ChatCharacter>& attacker)
{
    auto draw = SendBuffer::Alloc();
    
    //_owner->SendPacket(attacker->SessionId(),draw);
    
    auto view = std::views::all(attackRange.getSectors(*this))| GetValidSectorView();
    ranges::for_each(view,[this,&attackRange,&attacker,damage](playerSector& sector)
    {
        for(auto& player : sector)
        {
            if(player == attacker)
                continue;

            if(!attackRange.Contains(player->Location()))
                continue;

            player->Hit(damage);
        }
    });
}


void psh::GameMap::BroadcastDisconnect(const shared_ptr<Player>& target)
{
    const auto curSector = GetSector(target->Location());
    //삭제하고 
    RemovePlayer(curSector,target);
    //알린다.

    auto delBuffer = SendBuffer::Alloc();

    MakeGame_ResDestroyActor(delBuffer,target->ObjectId(),false);
    Broadcast(target->Location(),delBuffer);
}

void psh::GameMap::Broadcast(FVector location, SendBuffer& buffer)
{
    const auto targetSector = GetSector(location);
    SendToSectors(targetSector,buffer,BROADCAST,nullptr);
}


void psh::GameMap::SendToSectors(const Sector& targetSector,SendBuffer& buffer, const std::span<const psh::Sector> offsets, const shared_ptr<psh::
                                     Player>& exclude)
{
    auto broadcastSectors = GetSituatedSectorView(targetSector,offsets);
    ranges::for_each(broadcastSectors,[ this,&buffer,&exclude](playerSector& sector)
    {
        for(auto& player : sector)
        {
            if(player == exclude)
                continue;
            
            _owner->SendPacket(player->SessionId(), buffer,0);
        }
    });
}

psh::Sector IndexFromDiff(const psh::Sector sectorDiff)
{
    return psh::Sector(sectorDiff.x + 1,sectorDiff.y+1);
}

//플레이어를 실제로 이동시키는 함수.
void psh::GameMap::BroadcastIfSectorChange(const shared_ptr<ChatCharacter>& target,const FVector oldLocation, const FVector newLocation)
{
    const auto oldSector = GetSector(oldLocation);
    const auto newSector = GetSector(newLocation);
    
    if(oldSector == newSector)
    {
        return;
    }
    //섹터가 변경되었으면
    PRO_BEGIN("SectorChange")

    const auto sectorDiff = Clamp(newSector - oldSector,-1,1);
    const auto sectorIndex = IndexFromDiff(sectorDiff);
    
    //이전 섹터에서 지우고
    RemovePlayer(oldSector,target);
    
    //나를 삭제하라는 패킷을 만들고
    auto destroyThisPlayer = SendBuffer::Alloc();
    MakeGame_ResDestroyActor(destroyThisPlayer,target->ObjectId(),false);
    
    //나에게 삭제할 플레이어 알리는 패킷 모음
    vector<SendBuffer> toDelete;
    toDelete.reserve(4);
    toDelete.push_back(SendBuffer::Alloc());
    //삭제해야 하는 섹터들의 플레이어들을 순회하면서
    
    ApplyToPlayerInSector(oldSector,DeleteTable[sectorIndex.x][sectorIndex.y]
        ,[&destroyThisPlayer,&toDelete,this](const auto& player)
    {
            //삭제하라고 전송하고
            _owner->SendPacket(player->SessionId(),destroyThisPlayer);

            if (toDelete.back().CanPushSize() < 111)
                toDelete.push_back(SendBuffer::Alloc());
            //섹터에 있는 플레이어들을 삭제하라는 패킷을 만든다.
            MakeGame_ResDestroyActor(toDelete.back(), player->ObjectId(), false);
    });
    

    //만들어진 패킷이 있으면 나에게 전송한다.
    if(toDelete.back().Size() !=0)
    {
        _owner->SendPackets(target->SessionId(),toDelete);
    }


    //새로 정보를 받아와야 하는 섹터를 순회하면서
    //플레이어들을 구하고
    //서로 정보 받아온 후
    //해당 섹터로 이동하고
    //만들어진 패킷이 있으면 나에게 전송한다. 
    
    auto createThis = SendBuffer::Alloc();
    target->GetInfo(createThis,false);
    
    vector<SendBuffer> toCreate;
    toCreate.reserve(4);
    toCreate.push_back(SendBuffer::Alloc());

    ApplyToPlayerInSector(newSector,AddTable[sectorIndex.x][sectorIndex.y]
        ,[&createThis,&toCreate,this](auto& player)
    {
            //생성 전송하고
            _owner->SendPacket(player->SessionId(),createThis);
            //정보 추가

            if (toCreate.back().CanPushSize() < 111)
                toCreate.push_back(SendBuffer::Alloc());

            player->GetInfo(toCreate.back(),false);
    });

    //만들어진 패킷이 있으면 나에게 전송한다.
    if(toCreate.back().Size() != 0)
    {
        _owner->SendPackets(target->SessionId(), toCreate);
    }
    
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



psh::FVector psh::GameMap::GetRandomLocation() const
{
    return {rand() % (MAP_SIZE - 100) + 50.0f,rand() % (MAP_SIZE-100) + 50.0f  }; 
}



void psh::GameMap::ApplyToPlayerInSector(const Sector target, const std::span<const psh::Sector> offsets,
                                         const std::function<void(shared_ptr<Player>&)>& toInvoke)
{
    auto sendSectors = GetSituatedSectorView(target,offsets);

    ranges::for_each(sendSectors,[ this,&toInvoke](playerSector& sector)
    {
        for(auto& player : sector)
        {
            toInvoke(player);
        }
    });
}

void psh::GameMap::RemovePlayer(const shared_ptr<psh::Player>& target)
{
    const auto removeSector = GetSector(target->Location());

    RemovePlayer(removeSector,target);
}

void psh::GameMap::RemovePlayer(const Sector curSector, const shared_ptr<Player>& target)
{
    //_map[curSector.y][curSector.x].erase(remove(_map[curSector.y][curSector.x].begin(),_map[curSector.y][curSector.x].end(), target),_map[curSector.y][curSector.x].end());
   _map[curSector.x][curSector.y].erase(target);
}
        