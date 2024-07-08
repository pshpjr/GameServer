#pragma once
#include <algorithm>
#include <functional> // include this for std::hash

namespace psh
{
    //sector의 유효 검사는 Map에서
    struct Sector
    {
        bool operator==(const Sector& other) const
        {
            return x == other.x
                   && y == other.y;
        }

        bool operator!=(const Sector& other) const
        {
            return !(*this == other);
        }

        Sector operator-(const Sector& other) const
        {
            return {static_cast<short>(x - other.x), static_cast<short>(y - other.y)};
        }

        Sector operator+(const Sector& other) const
        {
            return {static_cast<short>(x + other.x), static_cast<short>(y + other.y)};
        }

        short x;
        short y;
        friend Sector Clamp(Sector target, short min, short max);
    };

    inline Sector Clamp(Sector target, short min, short max)
    {
        return {std::clamp(target.x, min, max), std::clamp(target.y, min, max)};
    }
}

// Specializing std::hash for Sector structure in the std namespace
namespace std
{
    template <>
    struct hash<psh::Sector>
    {
        size_t operator()(const psh::Sector& sector) const noexcept
        {
            return std::hash<short>()(sector.x) ^ (std::hash<short>()(sector.y) << 1);
        }
    };
}
