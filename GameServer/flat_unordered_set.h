#pragma once
#include <vector>

template <typename T>
class flat_unordered_set
{
    static constexpr char BUCKET_COUNT = 32;
    static constexpr char INDEX_MASK = BUCKET_COUNT - 1;

    static_assert( ( BUCKET_COUNT & INDEX_MASK ) == 0 );
    
public:
    class iterator
    {
    private:
        typename std::vector<std::vector<T>>::iterator outer;
        typename std::vector<std::vector<T>>::iterator outer_end;
        typename std::vector<T>::iterator inner;
        
    public:
        iterator(typename std::vector<std::vector<T>>::iterator outer,
                 typename std::vector<std::vector<T>>::iterator outer_end,
                 typename std::vector<T>::iterator inner)
        : outer{outer}, outer_end{outer_end}, inner(inner) {
            if (this->outer != this->outer_end && this->inner == this->outer->end()) {
                ++(*this);
            }
        }

        T& operator*() { return *inner; }
        T* operator->() { return &(*inner); }

        bool operator==(const iterator& other) const 
        {
            // 두 이터레이터가 모두 벡터의 끝을 가리키는지 확인 
            if (outer == outer_end && other.outer == other.outer_end) 
                return true;
            // 두 이터레이터가 같은 outer 및 inner 위치를 가리키는지 확인 
            else
                if (outer == other.outer && inner == other.inner) 
                return true;
            // 그 외의 경우, 이 두 이터레이터는 같지 않습니다. 
            else return false; 
        } 

        bool operator!=(const iterator& other) const {
            return !operator==(other);
        }

        iterator& operator++() {
            ++inner;
            while (outer != outer_end && (outer->empty() || inner == outer->end())) {
                ++outer;
                if (outer != outer_end) {
                    inner = outer->begin();
                }
            }
            return *this;
        }
    };
    

    
    flat_unordered_set():_data(BUCKET_COUNT,std::vector<T>())
    {
        for(auto& bucket : _data)
        {
            bucket.reserve(5);
        }
    }

    void insert(T&& data)
    {
        _size++;
        _data[hasher(data)&INDEX_MASK].push_back(std::forward<T>(data));
    }
    
    void insert(T& data)
    {
        _size++;
        auto hashResult = hasher(data)&INDEX_MASK;
        _data[hashResult].push_back(data);
    }

    void erase(T& data)
    {
        _size--;
        _data[hasher(data)&INDEX_MASK].erase(remove(_data[hasher(data)&INDEX_MASK].begin(),_data[hasher(data)&INDEX_MASK].end(),data),_data[hasher(data)&INDEX_MASK].end());
    }
    
    iterator begin() {
        auto outer_iter = _data.begin();
        while (outer_iter != _data.end() && outer_iter->empty()) {
            ++outer_iter;
        }
        if (outer_iter == _data.end()) {
            return end();
        }
        else {
            return iterator(outer_iter, _data.end(), outer_iter->begin());
        }
    }

    iterator end() {
        return iterator(_data.end(), _data.end(), {});
    }
    int size()
    {
        return _size;
    }
private:
    std::vector<std::vector<T>> _data;
    std::hash<T> hasher;
    int _size = 0;
};
