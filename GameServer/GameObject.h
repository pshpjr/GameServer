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

    //게임 오브젝트 생성 시 필요한 데이터들 묶음
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

        // 해당 객체 생성 패킷 만들 때 필요한 정보 추가
        // 부모 함수 호출 후 원하는 데이터 추가하기.
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

        //필드에 있다면 valid, 없다면 invalid인 걸로 관리중
        //TODO:깔끔하게 수정 가능할 듯
        void Valid(const bool inMap)
        {
            _inMap = inMap;
        }

        //그룹 이동시 맵에서 빼는 처리가 필요해서 추가함.
        //그 외의 경우에는 삭제시 맵에서 제거해도 됨.
        //그룹 이동 요청했다면 다른 애들의 패킷 받으면 안 됨.
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
