#pragma once
#include <chrono>

class Timer {

public:
    using ms = std::chrono::milliseconds;
    void Reset(const ms lifeMs) {
        _life = lifeMs;
        _elapsed = ms(0);
    }

    bool Update(const ms elapsed) {
        AddElapsed(elapsed);
        return IsExpired();
    }

    void AddElapsed(const ms elapsed) {
        _elapsed += elapsed;
    }

    [[nodiscard]] bool IsExpired() const noexcept {
        return _elapsed > _life;
    }

private:
    ms _life = ms(0);
    ms _elapsed = ms(0);
};
