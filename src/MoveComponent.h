
#ifndef MOVABLE_H
#define MOVABLE_H

#include "FVector.h"
#include "Sector.h"
#include "Updatable.h"

namespace psh
{
    class Field;
    class GameObject;

    /**
     * Movable 컴포넌트는 GameObject의 이동 기능을 담당합니다.
     * GameObject의 참조를 직접 사용하여 오버헤드 최소화.
     */
    class MoveComponent final : public Updatable {
    public:
        void Update(int delta) override;

        explicit MoveComponent(GameObject &owner, float baseSpeed);

        void MoveStart(const FVector &destination);

        void MoveStop();

        void UpdateLocation(int delta);

        // 섹터 이동하면 새로 만들고, 삭제할 애들 받아오는 함수
        // 플레이어만 의미 있음.
        void RefreshSectorObjects(Field &field, Sector delta) const;

        void BroadcastSectorChange(Sector delta) const;

        [[nodiscard]] bool IsMoving() const;

        [[nodiscard]] FVector Destination() const;

    private:
        GameObject &_owner;
        FVector _moveDirection{0, 0};
        FVector _destination{0, 0};
        bool _isMoving = false;
        float _baseSpeedPerMs{};
        float _moveSpeedPerMs{};
    };
}
#endif //MOVABLE_H
