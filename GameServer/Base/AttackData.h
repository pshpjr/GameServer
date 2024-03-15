#pragma once
#include "Range.h"

struct AttackData
{
    psh::Range* Range;
    int Damage;
    psh::ObjectID Attacker;
    psh::eCharacterGroup AttackerType;
};
