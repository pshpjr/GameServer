#pragma once
#include "../GameMap.h"
class Group;

namespace psh
{
    class Player;

    class GameObject;
    
    class ObjectManager
    {
    public:
        ObjectManager(GroupCommon& owner, GameMap<shared_ptr<Player>>& playerMap)
            : _owner(owner)
            , _playerMap(playerMap)
        {
        }
        virtual ~ObjectManager();
        virtual void Update(int deltaMs){}
        void SpawnActor(const shared_ptr<psh::GameObject>& actor,AttackManager* attackManager);
        virtual void OnActorDestroy(GameObject& actor);
        void DestroyActor(const shared_ptr<psh::GameObject>& actor, FVector location, const std::span<const Sector> offsets, bool isDead, bool notifySelf);
        void RequestMove(const shared_ptr<psh::GameObject>& actor, psh::FVector nextLocation);
        ObjectID NextObjectId()
        {
            return ++_objectID;
        }
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
        
    private:
        static Sector TableIndexFromDiff(const Sector sectorDiff)
        {
            return Sector(sectorDiff.x + 1, sectorDiff.y + 1);
        }
        
        void CreateActor(const shared_ptr<psh::GameObject>& actor
                         , const psh::FVector location
                         , std::span<const Sector> offsets
                         , const bool isSpawn
                         , const bool notifySelf
            );


        ObjectID _objectID = 0;
    };
    
}
