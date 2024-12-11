#pragma once


#include <vector>
#include <cstring>

namespace Emulator {
    template<typename Type, int N, int K>
    struct Array {
        Type arr[N][K]{};

        void init(int n, int k) {}

        void clear() {
            std::memset(arr, 0, sizeof(arr));
        }

        Type *operator[](int n) {
            return arr[n];
        }

        Array &operator=(const Array &other) {
            if (this == &other) {
                return *this;
            }
            std::memcpy(arr, other.arr, sizeof(arr));
            return *this;
        }
    };

    template<typename Type>
    struct Array<Type, -1, -1> {
        std::vector<std::vector<Type>> arr{};

        void init(int n, int k) {
            arr.assign(n, std::vector<Type>(k, Type{}));
        }

        void clear() {
            arr = std::vector(arr.size(), std::vector<Type>(arr[0].size(), Type{}));
        }

        std::vector<Type> &operator[](int n) {
            return arr[n];
        }

        Array &operator=(const Array &other) {
            if (this == &other) {
                return *this;
            }
            arr = other.arr;
            return *this;
        }
    };
}