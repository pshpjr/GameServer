
#include "NormalQuadTree.h"


void NormalQuadTree::Iterate(psh::FVector start, psh::FVector end, const std::function<void(psh::Player&)>& toInvoke)
{
    GetPlayersInRange(root, start, end, toInvoke);

}
