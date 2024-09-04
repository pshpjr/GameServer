#pragma once
#include <random>

namespace psh
{
    struct RandomUtil
    {
        inline static thread_local auto gen = std::mt19937(0);

        static void SRand(const int seed)
        {
            gen = std::mt19937(seed);
        }

        static int Rand(const int min, const int max)
        {
            std::uniform_int_distribution dis(min, max);
            return dis(gen);
        }
    };
}
