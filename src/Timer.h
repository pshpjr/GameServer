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
            AddElapsed(delta);
        }

        void AddElapsed(int elapsed)
        {
            _elapsed += elapsed;
        }

        [[nodiscard]] bool IsExpired() const noexcept
        {
            return _elapsed >= _life;
        }

    private:
        int _life = 0;
        int _elapsed = 0;
    };
}

