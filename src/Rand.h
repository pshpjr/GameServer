#pragma once
#include <random>

namespace psh
{
    struct RandomUtil
    {
        //fix seed for debug
        inline static thread_local auto gen = std::mt19937(0); // NOLINT(*-msc51-cpp)

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
