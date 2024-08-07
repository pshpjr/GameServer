﻿#pragma once
#include "GameMap.h"
class Group;

namespace psh
{
    class Player;

    class GameObject;
    
    class ObjectManager
    {
    public:
        enum removeResult : char
        {
          Move,
            Die,
            GroupChange
        };
        
        ObjectManager(GroupCommon& owner, GameMap<shared_ptr<Player>>& playerMap, AttackManager* attackManager)
            : _owner(owner)
            , _playerMap(playerMap),
            _attackManager(attackManager)
        {
        }
        virtual ~ObjectManager();
        virtual void Update(int deltaMs){}
        void SpawnActor(const shared_ptr<psh::GameObject>& actor,AttackManager* attackManager);
        void DestroyActor(shared_ptr<psh::GameObject> actor);
        void RemoveFromMap(const shared_ptr<psh::GameObject>& actor, FVector location, const std::span<const Sector> offsets, bool isDead, bool notifySelf, char
                          cause);
        void RequestMove(const shared_ptr<psh::GameObject>& actor, psh::FVector nextLocation);
        ObjectID NextObjectId()
        {
            return ++_objectID;
        }
        void CleanupDestroyWait()
        {
            for(auto& obj : _deleteWait)
            {

                obj->OnDestroy();
                CleanupActor(obj.get());
            }
            _deleteWait.clear();
        }
        virtual void CleanupActor(GameObject* actor) {}
        

    protected:
        /**
 * \brief objectManager에서 관리중인 Map들을 container에 담아준다. 해당 map에 있는 액터의 정보를 받아올 때 사용.
 * \param container 
 */
        virtual void GetAllGameMap(std::vector<GameMap<shared_ptr<GameObject>>*>& container);
        /**
         * \brief 해당 액터가 있어야 할 map에 삽입해야 한다. 
         * \param actor 
         */
        virtual void InsertInGameMap(const shared_ptr<psh::GameObject>& actor, FVector location);

        /**
         * \brief 해당 액터가 있는 map에서 삭제한다.  
         * \param actor 
         */
        virtual void EraseToGameMap(const shared_ptr<psh::GameObject>& actor, FVector location);
        virtual void OnActorMove(const shared_ptr<GameObject>& actor){}
        GroupCommon& _owner;
        GameMap<shared_ptr<Player>>& _playerMap;
        list<shared_ptr<psh::GameObject>>_deleteWait;
        AttackManager* _attackManager;
    private:
        static Sector TableIndexFromDiff(const Sector sectorDiff)
        {
            return Sector(sectorDiff.x + 1, sectorDiff.y + 1);
        }
        
        void InsertInMap(const shared_ptr<psh::GameObject>& actor
                         , const psh::FVector location
                         , std::span<const Sector> offsets
                         , const bool isSpawn
                         , const bool notifySelf
            );


        ObjectID _objectID = 0;

    };
    
}
