#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

template <typename T, std::size_t N> struct recycler {
    void free(T *ptr) {
        assert(_waste_count < N);
        assert(_waste[_waste_count] == nullptr);
        _waste[_waste_count++] = ptr;
    }

    T *acquire() {
        if (_waste_count > 0) {
            --_waste_count;
            return _waste[_waste_count];
        } else if (_data_count < N) {
            return &_data[_data_count++];
        }
        return nullptr;
    }

    const T &operator[](size_t index) const { return _data[index]; }
    T &operator[](size_t index) { return _data[index]; }
    size_t count() const { return _data_count; };
    T *data() { return _data; }

  private:
    T _data[N] = {};
    size_t _data_count{0};

    T *_waste[N] = {};
    size_t _waste_count{0};
};