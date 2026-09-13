#pragma once
#include <algorithm>
#include <functional>
#include <iostream>
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

        bool operator<(const Sector& other) const
        {
            if (x < other.x)
            {
                return true;
            }
            if (x == other.x)
            {
                return y < other.y;
            }
            return false;
        }

        Sector operator-(const Sector& other) const
        {
            return {static_cast<short>(x - other.x), static_cast<short>(y - other.y)};
        }

        Sector operator+(const Sector& other) const
        {
            return {static_cast<short>(x + other.x), static_cast<short>(y + other.y)};
        }


        friend static Sector Clamp(const Sector target, const short min, const short max)
        {
            return {std::clamp(target.x, min, max), std::clamp(target.y, min, max)};
        }

        friend std::ostream& operator<<(std::ostream& out, const Sector& sector)
        {
            out << "( X: " << sector.x << " Y:" << sector.y << ")";
            return out;
        }

        short x;
        short y;
    };
}

// sector hash 연산.
template <>
struct std::hash<psh::Sector>
{
    size_t operator()(const psh::Sector& sector) const noexcept
    {
        return std::hash<short>()(sector.x) ^ (std::hash<short>()(sector.y) << 1);
    }
};


