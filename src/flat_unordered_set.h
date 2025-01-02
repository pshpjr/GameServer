/*
 *
 * 내부적으로 2차원 벡터로 데이터를 관리하는 unordered_set
 * 캐시 히트율을 높혀 순회 성능을 높혀보기 위한 것
 * 테스트 결과 의미 없어서 폐기함
 *
 *
 */



#pragma once
#include <ranges>
#include <vector>
#include "Macro.h"


template <typename T>
[[deprecated]]
class flat_unordered_set_iterator
{
    using inner_iterator = std::conditional_t<std::is_const_v<T>,
                                              typename std::vector<std::remove_const_t<T>>::const_iterator,
                                              typename std::vector<std::remove_const_t<T>>::iterator>;
    typename std::vector<std::vector<T>>::iterator _outer;
    typename std::vector<std::vector<T>>::iterator _outerEnd;
    inner_iterator _inner;

public:
    flat_unordered_set_iterator(typename std::vector<std::vector<T>>::iterator outer
        , typename std::vector<std::vector<T>>::iterator outer_end
        , typename std::vector<T>::iterator inner)
        : _outer{outer}
        , _outerEnd{outer_end}
        , _inner(inner)
    {
        if (this->_outer != this->_outerEnd && this->_inner == this->_outer->end())
        {
            ++(*this);
        }
    }

    ~flat_unordered_set_iterator() = default;

    flat_unordered_set_iterator(const flat_unordered_set_iterator& other) = default;

    flat_unordered_set_iterator(flat_unordered_set_iterator&& other) noexcept
        : _outer{std::move(other._outer)}
        , _outerEnd{std::move(other._outerEnd)}
        , _inner{std::move(other._inner)}
    {
    }


    flat_unordered_set_iterator& operator=(flat_unordered_set_iterator other)
    {
        using std::swap;
        swap(*this, other);
        return *this;
    }

    const T& operator*() const
    {
        return *_inner;
    }

    const T* operator->() const
    {
        return &(*_inner);
    }

    T& operator*()
    {
        return *_inner;
    }

    T* operator->()
    {
        return &(*_inner);
    }

    flat_unordered_set_iterator& operator++()
    {
        if (_outer != _outerEnd)
        {
            ++_inner;
            while (_outer != _outerEnd && _inner == _outer->end())
            {
                ++_outer;
                if (_outer != _outerEnd)
                {
                    _inner = _outer->begin();
                }
            }
        }
        return *this;
    }

    flat_unordered_set_iterator operator++(int)
    {
        flat_unordered_set_iterator tmp = *this;
        operator++();
        return tmp;
    }

    friend bool operator==(const flat_unordered_set_iterator& lhs, const flat_unordered_set_iterator& rhs)
    {
        return lhs._outer == rhs._outer &&
            (lhs._outer == lhs._outerEnd || lhs._inner == rhs._inner);
    }

    friend bool operator!=(const flat_unordered_set_iterator& lhs, const flat_unordered_set_iterator& rhs)
    {
        return !(lhs == rhs);
    }
};

template <typename T>
class flat_unordered_set
{
    static constexpr char BUCKET_COUNT = 32;
    static constexpr char INDEX_MASK = BUCKET_COUNT - 1;

    static_assert((BUCKET_COUNT & INDEX_MASK) == 0);
    using iterator = flat_unordered_set_iterator<T>;
    using const_iterator = flat_unordered_set_iterator<const T>;

public:
    flat_unordered_set()
        : _data(BUCKET_COUNT, std::vector<T>())
    {
        for (auto& bucket : _data)
        {
            bucket.reserve(5);
        }
    }

    void insert(T&& data)
    {
        _size++;
        GetBucket(data).push_back(std::forward<T>(data));
    }


    void insert(const T& data)
    {
        _size++;
        GetBucket(data).push_back(data);
    }

    void erase(const T& data)
    {
        auto& bucket = GetBucket(data);
        _size--;
        ASSERT_CRASH(_size>=0, L"InvalidEraseSize");
        auto bucketSize = bucket.size();

        bucket.erase(remove(bucket.begin()
                , bucket.end(), data)
            , bucket.end());
        ASSERT_CRASH(bucket.size() == bucketSize - 1, L"bucket isn't contain data");
    }

    iterator begin()
    {
        auto outer_iter = _data.begin();
        while (outer_iter != _data.end() && outer_iter->empty())
        {
            ++outer_iter;
        }
        if (outer_iter == _data.end())
        {
            return end();
        }
        return iterator(outer_iter, _data.end(), outer_iter->begin());
    }

    iterator end()
    {
        auto outer_iter = _data.end();
        return iterator(outer_iter, outer_iter, {});
    }

    const_iterator begin() const
    {
        auto outer_iter = _data.cbegin();
        while (outer_iter != _data.cend() && outer_iter->empty())
        {
            ++outer_iter;
        }
        if (outer_iter == _data.cend())
        {
            return end();
        }
        return const_iterator(outer_iter, _data.cend(), outer_iter->begin());
    }

    const_iterator end() const
    {
        auto outer_iter = _data.cend();
        return const_iterator(outer_iter, outer_iter, {});
    }

    [[nodiscard]] int size() const
    {
        return _size;
    }

    auto GetView()
    {
        return std::ranges::subrange<iterator>(begin(), end());
    }

private:
    std::vector<T>& GetBucket(const T& data)
    {
        return _data[hasher(data) & INDEX_MASK];
    }

    std::vector<std::vector<T>> _data;
    std::hash<T> hasher;
    int _size = 0;
};
