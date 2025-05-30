#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

template <typename T, std::size_t N> struct recycler {
    void free(T *ptr) {
        assert(_waste_count < N);
        _waste[_waste_count++] = ptr;
    }

    T *acquire() {
        if (_waste_count > 0) {
            --_waste_count;
            T *t = _waste[_waste_count];
            _waste[_waste_count] = nullptr;
            return t;
        } else if (_data_count < N) {
            return &_data[_data_count++];
        }
        assert(false); // full
        return nullptr;
    }

    const T &operator[](size_t index) const { return _data[index]; }
    T &operator[](size_t index) { return _data[index]; }
    size_t size() const { return N; }
    size_t count() const { return _data_count; };
    T *data() { return _data; }

    T &at(size_t idx) {
        if (idx >= _data_count) {
            throw std::out_of_range("recycler::at(): index is out of range");
        }
        return _data[idx];
    }

    void clear() {
        for (size_t i{0}; i < N; ++i) {
            _data[i] = {};
            _waste[i] = nullptr;
        }
        _data_count = 0;
        _waste_count = 0;
    }

  private:
    T _data[N] = {};
    size_t _data_count{0};

    T *_waste[N] = {};
    size_t _waste_count{0};
};