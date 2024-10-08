#pragma once
#include <fstream>
#include <functional>
#include <memory>

#include "FVector.h"
#include "Macro.h"

namespace psh
{
    struct Sector;

    class Range {
        IS_INTERFACE(Range)

        [[nodiscard]] virtual bool Contains(FVector p) const noexcept = 0;

        /**
         * 
         * @return 해당 객체가 있는 섹터를 판별하기 위한 점들을 반환. 이로 구한 섹터는 중복될 수 있음.
         */
        [[nodiscard]] virtual std::list<FVector> GetCoordinates() const = 0;

        [[nodiscard]] virtual Range &operator+=(FVector vector) = 0;

        virtual void DrawRangeIntoBuffer(SendBuffer &buffer) const
        {
        }

        virtual void printInfo(std::ostream &os) const
        {
        };

        friend std::ostream &operator<<(std::ostream &stream, const Range &range)
        {
            range.printInfo(stream);
            return stream;
        }
    };


    class SquareRange final : public Range {
    public:
        explicit SquareRange(FVector p1);

        SquareRange(FVector p1, FVector p2);

        void Rotate(FVector rotation, FVector origin = {0, 0}) noexcept;

        [[nodiscard]] bool Contains(FVector p) const noexcept override;

        [[nodiscard]] [[deprecated]] bool oldContaion(FVector p) const noexcept;


        [[nodiscard]] std::list<FVector> GetCoordinates() const override;

        [[nodiscard]] SquareRange &operator+=(FVector vector) override;

        void DrawRangeIntoBuffer(SendBuffer &buffer) const override;

        void printInfo(std::ostream &os) const override;

    private:
        struct Rotation {
            FVector rotation;
            FVector origin;
        };

        FVector _p1{};
        FVector _p2{};
        std::array<FVector, 4> _points{};
        std::list<FVector> _midPoints{};
        std::list<Rotation> _rotations{};
    };


    class CircleRange final : public Range {
    public:
        CircleRange(const FVector &point, float radius);

        [[nodiscard]] bool Contains(FVector p) const noexcept override;

        [[nodiscard]] std::list<FVector> GetCoordinates() const override;

        [[nodiscard]] Range &operator+=(FVector vector) override;

    private:
        FVector _point;
        float _radius;
        std::list<FVector> _keyPoints;
    };

    using RangeUnique = std::unique_ptr<Range, std::function<void(Range *)> >;
}
