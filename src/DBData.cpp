#include "DBData.h"
#include "Player.h"

void psh::DBData::SaveAll(Player& player)
{
    _location = player.Location();
    _hp = player.Hp();

    _hp = std::clamp(_hp, 0, 100);
    ASSERT_CRASH(0<=_hp && _hp <= 100);
}

void psh::DBData::AddCoin(char value)
{
    _coin += value;
}
