#pragma once


#include <vector>
#include <cstring>

namespace Emulator {
    template<typename Type, int N_val, int K_val>
    struct Array {
        Type arr[N_val][K_val]{};
        int N = N_val;
        int K = K_val;

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
        int N = 0;
        int K = 0;

        void init(int n, int k) {
            N = n;
            K = k;

            if constexpr (std::is_copy_constructible_v<Type>) {
                arr.assign(n, std::vector<Type>(k, Type{}));
            } else {
                arr.resize(n);
                for (auto &line: arr) {
                    std::vector<Type> tmp(k);
                    line.swap(tmp);
                }
            }
        }

        void clear() {
            arr = std::vector(N, std::vector<Type>(K, Type{}));
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

    template<typename Type, int N, int K>
    void load_array(Array<Type, N, K> &arr, std::ifstream &file, int n, int k) {
        arr.init(n, k);
        for (int i = 0; i < arr.N; i++) {
            for (int j = 0; j < arr.K; j++) {
                if constexpr (std::is_same_v<Type, char>) {
                    int temp;
                    file >> temp;
                    arr[i][j] = char(temp);
                } else {
                    double temp;
                    file >> temp;
                    arr[i][j] = temp;
                }
            }
        }
    }

    template<typename Type, int N, int K>
    void load_array(Array<std::array<Type, 4>, N, K> &arr, std::ifstream &file, int n, int k) {
        arr.init(n, k);
        for (int i = 0; i < arr.N; i++) {
            for (int j = 0; j < arr.K; j++) {
                double temp[4];
                file >> temp[0] >> temp[1] >> temp[2] >> temp[3];
                arr[i][j] = {Type(temp[0]), Type(temp[1]), Type(temp[2]), Type(temp[3])};
            }
        }
    }

}