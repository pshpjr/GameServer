#pragma once
#include <random>

namespace psh
{
    //랜덤 함수 래핑
    struct RandomUtil {
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

        static float Rand(const float min, const float max)
        {
            std::uniform_real_distribution dis(min, max);
            return dis(gen);
        }
    };
}
