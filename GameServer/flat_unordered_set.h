#pragma once
#include <vector>
#include <ranges>

template <typename T>
class flat_unordered_set_iterator
{
private:
    using inner_iterator = std::conditional_t<std::is_const_v<T>,
                                              typename std::vector<std::remove_const_t<T>>::const_iterator,
                                              typename std::vector<std::remove_const_t<T>>::iterator>;
    typename std::vector<std::vector<T>>::iterator outer;
    typename std::vector<std::vector<T>>::iterator outer_end;
    inner_iterator inner;

public:
    flat_unordered_set_iterator(typename std::vector<std::vector<T>>::iterator outer
                                , typename std::vector<std::vector<T>>::iterator outer_end
                                , typename std::vector<T>::iterator inner)
        : outer{outer}
        , outer_end{outer_end}
        , inner(inner)
    {
        if (this->outer != this->outer_end && this->inner == this->outer->end())
        {
            ++(*this);
        }
    }

    ~flat_unordered_set_iterator() = default;

    flat_unordered_set_iterator(const flat_unordered_set_iterator& other) = default;

    flat_unordered_set_iterator(flat_unordered_set_iterator&& other) noexcept
        : outer{std::move(other.outer)}
        , outer_end{std::move(other.outer_end)}
        , inner{std::move(other.inner)}
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
        return *inner;
    }

    const T* operator->() const
    {
        return &(*inner);
    }

    T& operator*()
    {
        return *inner;
    }

    T* operator->()
    {
        return &(*inner);
    }

    flat_unordered_set_iterator& operator++()
    {
        if (outer != outer_end)
        {
            ++inner;
            while (outer != outer_end && inner == outer->end())
            {
                ++outer;
                if (outer != outer_end)
                {
                    inner = outer->begin();
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
        return lhs.outer == rhs.outer &&
               (lhs.outer == lhs.outer_end || lhs.inner == rhs.inner);
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
    flat_unordered_set(): _data(BUCKET_COUNT, std::vector<T>())
    {
        for (auto& bucket : _data)
        {
            bucket.reserve(5);
        }
    }

    void insert(T&& data)
    {
        _size++;
        _data[hasher(data) & INDEX_MASK].push_back(std::forward<T>(data));
    }

    void insert(const T& data)
    {
        _size++;
        auto hashResult = hasher(data) & INDEX_MASK;
        _data[hashResult].push_back(data);
    }

    void erase(const T& data)
    {
        _size--;
        _data[hasher(data) & INDEX_MASK].erase(remove(_data[hasher(data) & INDEX_MASK].begin()
                                                      , _data[hasher(data) & INDEX_MASK].end(), data)
                                               , _data[hasher(data) & INDEX_MASK].end());
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

    int size() const
    {
        return _size;
    }

    // auto GetView()
    // {
    //     return views::all(this);
    // }
private:
    std::vector<std::vector<T>> _data;
    std::hash<T> hasher;
    int _size = 0;
};
