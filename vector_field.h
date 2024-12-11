#pragma once

#include <utility>
#include <ranges>
#include <cassert>

#include "utilities.h"
#include "static_array.h"

namespace Emulator {
    template<typename T, int N, int K>
    struct VectorField {
        Array<std::array<T, deltas.size()>, N, K> v;

        T &add(int x, int y, int dx, int dy, T dv) {
            return get(x, y, dx, dy) += dv;
        }

        T &get(int x, int y, int dx, int dy) {
            size_t i = std::ranges::find(deltas, std::pair(dx, dy)) - deltas.begin();
            assert(i < deltas.size());
            return v[x][y][i];
        }
    };
}