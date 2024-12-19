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
            switch ((dx << 1) + dy) {
                case 1:
                    return v[x][y][3];
                case 2:
                    return v[x][y][1];
                case -1:
                    return v[x][y][2];
                default:
                    return v[x][y][0];
            }
        }
    };
}