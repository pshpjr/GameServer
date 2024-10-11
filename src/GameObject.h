#pragma once

#include <format>
#include <memory>
#include <PacketGenerated.h>
#include "ContentTypes.h"
#include "Updatable.h"

namespace psh
{
    // 전방 선언 (Forward Declarations)
    class Field;
    template <typename Key, typename T>
    class GameMap;

    struct GameObjectData
    {
        FVector location{0, 0};
        FVector direction{0, 0};
        float moveSpeedPerSec{200};
        eObjectType objectType = eObjectType::Object;
        TemplateID templateId = -1;
    };

    enum class removeReason : char
    {
        None
        , GroupChange
        , Move
        , Die
        , Disconnect
    };

    /**
     * 생성 소멸 알림은 외부에서 알아서 함.
     */
    class GameObject
        : public std::enable_shared_from_this<GameObject>
          , public Updatable
    {
        //컴포넌트만 객체 setter에 접근
        friend class MoveComponent;
        friend class MonsterAiComponent;

    public:
        // 생성자와 소멸자 (Constructors and Destructor)
        GameObject(Field& group, const GameObjectData& initData);

        ~GameObject() override;

        // 공용 멤버 함수 (Public Member Functions)
        virtual void MakeCreatePacket(SendBuffer& buffer, bool spawn) const;

        void Update(int delta) final;

        virtual void OnUpdate(int delta) {};

        //객체 생성, 소멸 관련
        void OnCreate();

        virtual void OnCreateImpl() {}

        void OnDestroy();

        virtual void OnDestroyImpl() {}

        //게터
        [[nodiscard]] FVector Location() const
        {
            return _location;
        }

        [[nodiscard]] ObjectID ObjectId() const
        {
            return _objectId;
        }

        [[nodiscard]] FVector ViewDirection() const
        {
            return _viewDirection;
        }

        [[nodiscard]] eObjectType ObjectType() const
        {
            return _objectType;
        }

        [[nodiscard]] bool Valid() const
        {
            return _inMap;
        }

        [[nodiscard]] GameMap<ObjectID, std::shared_ptr<GameObject>>* GetMap() const
        {
            return _map;
        }

        [[nodiscard]] Field& GetField() const
        {
            return _field;
        }

        //필드만 접근하는 setter 함수들. 얘도 빼낼 수 있을 것 같은데
        void Valid(const bool inMap)
        {
            _inMap = inMap;
        }

        void OutFromMapWhenGroupMove();
        void IntoMap(GameMap<ObjectID, std::shared_ptr<GameObject>>* map);

        void ObjectId(const ObjectID id)
        {
            _objectId = id;
        }

        //디버깅용
        friend std::ostream& operator<<(std::ostream& out, const GameObject& obj);

        void RemoveReason(removeReason reason)
        {
            _removeReason = reason;
        }

        [[nodiscard]] removeReason RemoveReason() const
        {
            return _removeReason;
        }

        [[nodiscard]] TemplateID TemplateId() const
        {
            return _templateId;
        }

    protected:
        //멤버
        Field& _field;
        GameMap<ObjectID, std::shared_ptr<GameObject>>* _map{nullptr};
        removeReason _removeReason{None};

    private:
        void Location(const FVector loc)
        {
            _location = loc;
        }

        void ViewDirection(FVector dir)
        {
            _viewDirection = dir;
        }


        ObjectID _objectId{};
        FVector _location{};
        FVector _viewDirection{};
        eObjectType _objectType = eObjectType::Object;
        TemplateID _templateId = 0;

        bool _inMap = false;
    };
}
