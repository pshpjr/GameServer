#pragma once
#include <algorithm>
#include <functional>
// API-compatible sequential sorting for this offline utility only.
namespace ppqsort {
namespace execution { inline constexpr int par = 0; }
template<class Iterator, class Compare = std::less<>>
void sort(int, Iterator begin, Iterator end, Compare compare = {}) {
    if (begin != end) std::sort(begin, end, compare);
}
}
