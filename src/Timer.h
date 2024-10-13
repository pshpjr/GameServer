#pragma once
#include "Updatable.h"

namespace psh
{
    class Timer : public Updatable
    {
    public:
        void Reset(int lifeMs)
        {
            _life = lifeMs;
            _elapsed = 0;
        }

        void Update(int delta) override
        {
            if (IsExpired())
            {
                return;
            }

            AddElapsed(delta);
        }

        void AddElapsed(int elapsed)
        {
            _elapsed += elapsed;
        }

        [[nodiscard]] bool IsExpired() const noexcept;


        [[nodiscard]] int Elapsed() const noexcept;

    private:
        int _life = 0;
        int _elapsed = 0;
    };

    inline bool Timer::IsExpired() const noexcept
    {
        return _elapsed >= _life;
    }

    inline int Timer::Elapsed() const noexcept
    {
        return _elapsed;
    }
}

