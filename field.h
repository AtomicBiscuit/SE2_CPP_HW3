#pragma once

#include <ranges>
#include <iomanip>
#include <cassert>
#include <utility>
#include <array>
#include <random>
#include <string>
#include <memory>
#include <algorithm>
#include <cstring>

#include "numbers.h"
#include "static_array.h"
#include "utilities.h"
#include "vector_field.h"


namespace Emulator {
    struct AbstractField {
        virtual void init(std::vector<std::vector<char>> f, int n, int k) = 0;

        virtual void next(int) = 0;

        virtual ~AbstractField() = default;
    };

    template<typename PType, typename VType, typename VFType, int N_val, int K_val>
    struct FieldEmulator : public AbstractField {
        int N = 0;
        int K = 0;

        Array<char, N_val, K_val == -1 ? -1 : K_val + 1> field{};

        VectorField<VType, N_val, K_val> velocity = {};
        VectorField<VFType, N_val, K_val> velocity_flow = {};

        Array<int64_t, N_val, K_val> dirs{};
        Array<int, N_val, K_val> last_use{};
        int UT = 0;

        PType rho[256];
        Array<PType, N_val, K_val> p{}, old_p{};

        constexpr FieldEmulator() = default;

        void init(std::vector<std::vector<char>> f, int n, int k) override {
            N = n;
            K = k;
            field.init(n, k);

            for (int i = 0; i < n; i++) {
                for (int j = 0; j < k; j++) {
                    field[i][j] = f[i][j];
                }
            }

            velocity.v.init(n, k);
            velocity_flow.v.init(n, k);
            dirs.init(n, k);
            last_use.init(n, k);
            p.init(n, k);
            old_p.init(n, k);

            rho[' '] = 0.01;
            rho['.'] = int64_t(1000);
            for (int x = 0; x < N; ++x) {
                for (int y = 0; y < K; ++y) {
                    if (field[x][y] == '#')
                        continue;
                    for (auto [dx, dy]: deltas) {
                        dirs[x][y] += (field[x + dx][y + dy] != '#');
                    }
                }
            }
        }

        std::tuple<VFType, bool, std::pair<int, int>> propagate_flow(int x, int y, VFType lim) {
            last_use[x][y] = UT - 1;
            VFType ret{};
            for (auto [dx, dy]: deltas) {
                int nx = x + dx, ny = y + dy;
                if (field[nx][ny] != '#' && last_use[nx][ny] < UT) {
                    VType cap = velocity.get(x, y, dx, dy);
                    VFType flow = velocity_flow.get(x, y, dx, dy);
                    if (fabs(flow - VFType(cap)) <= 0.0001) {
                        continue;
                    }
                    VFType vp = std::min(lim, VFType(cap) - flow);
                    if (last_use[nx][ny] == UT - 1) {
                        velocity_flow.add(x, y, dx, dy, vp);
                        last_use[x][y] = UT;
                        return {vp, true, {nx, ny}};
                    }
                    auto [t, prop, end] = propagate_flow(nx, ny, vp);
                    ret += t;
                    if (prop) {
                        velocity_flow.add(x, y, dx, dy, t);
                        last_use[x][y] = UT;
                        return {t, end != std::pair(x, y), end};
                    }
                }
            }
            last_use[x][y] = UT;
            return {ret, false, {0, 0}};
        }

        void propagate_stop(int x, int y, bool force = false) {
            if (!force) {
                for (auto [dx, dy]: deltas) {
                    int nx = x + dx, ny = y + dy;
                    if (field[nx][ny] != '#' && last_use[nx][ny] < UT - 1 && velocity.get(x, y, dx, dy) > int64_t(0)) {
                        return;
                    }
                }
            }
            last_use[x][y] = UT;
            for (auto [dx, dy]: deltas) {
                int nx = x + dx, ny = y + dy;
                if (field[nx][ny] == '#' || last_use[nx][ny] == UT || velocity.get(x, y, dx, dy) > int64_t(0)) {
                    continue;
                }
                propagate_stop(nx, ny);
            }
        }

        VType move_prob(int x, int y) {
            VType sum{};
            for (auto [dx, dy]: deltas) {
                int nx = x + dx, ny = y + dy;
                if (field[nx][ny] == '#' || last_use[nx][ny] == UT) {
                    continue;
                }
                VType v = velocity.get(x, y, dx, dy);
                if (v < int64_t(0)) {
                    continue;
                }
                sum += v;
            }
            return sum;
        }

        void swap(int x1, int y1, int x2, int y2) {
            std::swap(field[x1][y1], field[x2][y2]);
            std::swap(p[x1][y1], p[x2][y2]);
            std::swap(velocity.v[x1][y1], velocity.v[x2][y2]);
        }

        bool propagate_move(int x, int y, bool is_first) {
            last_use[x][y] = UT - is_first;
            bool ret = false;
            int nx = -1, ny = -1;
            do {
                std::array<VType, deltas.size()> tres;
                VType sum{};
                for (size_t i = 0; i < deltas.size(); ++i) {
                    auto [dx, dy] = deltas[i];
                    int forward_x = x + dx, forward_y = y + dy;
                    if (field[forward_x][forward_y] == '#' || last_use[forward_x][forward_y] == UT) {
                        tres[i] = sum;
                        continue;
                    }
                    VType v = velocity.get(x, y, dx, dy);
                    if (v < int64_t(0)) {
                        tres[i] = sum;
                        continue;
                    }
                    sum += v;
                    tres[i] = sum;
                }

                if (sum == int64_t(0)) {
                    break;
                }

                VType random_num = random01<VType>() * sum;
                size_t d = std::ranges::upper_bound(tres, random_num) - tres.begin();

                auto [dx, dy] = deltas[d];
                nx = x + dx;
                ny = y + dy;
                assert(velocity.get(x, y, dx, dy) > int64_t(0) && field[nx][ny] != '#' && last_use[nx][ny] < UT);
                ret = (last_use[nx][ny] == UT - 1 || propagate_move(nx, ny, false));
            } while (!ret);
            last_use[x][y] = UT;
            for (auto [dx, dy]: deltas) {
                int forward_x = x + dx, forward_y = y + dy;
                if (field[forward_x][forward_y] != '#' && last_use[forward_x][forward_y] < UT - 1 &&
                    velocity.get(x, y, dx, dy) < int64_t(0)) {
                    propagate_stop(forward_x, forward_y);
                }
            }
            if (ret and !is_first) {
                swap(x, y, nx, ny);
            }
            return ret;
        }

        void apply_external_forces() {
            for (int x = 0; x < N; ++x) {
                for (int y = 0; y < K; ++y) {
                    if (field[x][y] == '#')
                        continue;
                    if (field[x + 1][y] != '#')
                        velocity.add(x, y, 1, 0, g<VType>());
                }
            }
        }

        void apply_p_forces(PType &total_delta_p) {
            old_p = p;
            for (int x = 0; x < N; ++x) {
                for (int y = 0; y < K; ++y) {
                    if (field[x][y] == '#')
                        continue;
                    for (auto [dx, dy]: deltas) {
                        int nx = x + dx, ny = y + dy;
                        if (field[nx][ny] == '#' or old_p[nx][ny] >= old_p[x][y]) {
                            continue;
                        }
                        PType delta_p = old_p[x][y] - old_p[nx][ny];
                        PType force = delta_p;
                        VType &contr = velocity.get(nx, ny, -dx, -dy);
                        if (PType(contr) * rho[(int) field[nx][ny]] >= force) {
                            contr -= VType(force / rho[(int) field[nx][ny]]);
                            continue;
                        }
                        force -= PType(contr) * rho[(int) field[nx][ny]];
                        contr = int64_t(0);
                        velocity.add(x, y, dx, dy, VType(force / rho[(int) field[x][y]]));
                        p[x][y] -= force / dirs[x][y];
                        total_delta_p -= force / dirs[x][y];
                    }
                }
            }
        }

        void recalculate_p(PType &total_delta_p) {
            for (int x = 0; x < N; ++x) {
                for (int y = 0; y < K; ++y) {
                    if (field[x][y] == '#')
                        continue;
                    for (auto [dx, dy]: deltas) {
                        VType old_v = velocity.get(x, y, dx, dy);
                        VFType new_v = velocity_flow.get(x, y, dx, dy);
                        if (old_v > int64_t(0)) {
                            assert(VType(new_v) <= old_v);
                            velocity.get(x, y, dx, dy) = VType(new_v);
                            auto force = PType(old_v - VType(new_v)) * rho[(int) field[x][y]];
                            if (field[x][y] == '.')
                                force *= 0.8;
                            if (field[x + dx][y + dy] == '#') {
                                p[x][y] += force / dirs[x][y];
                                total_delta_p += force / dirs[x][y];
                            } else {
                                p[x + dx][y + dy] += force / dirs[x + dx][y + dy];
                                total_delta_p += force / dirs[x + dx][y + dy];
                            }
                        }
                    }
                }
            }
        }

        void next(int i) override {
            PType total_delta_p = int64_t(0);
            apply_external_forces();
            apply_p_forces(total_delta_p);

            velocity_flow.v.clear();
            bool prop;
            do {
                UT += 2;
                prop = false;
                for (int x = 0; x < N; ++x) {
                    for (int y = 0; y < K; ++y) {
                        if (field[x][y] == '#' or last_use[x][y] == UT) {
                            continue;
                        }
                        auto [t, local_prop, _] = propagate_flow(x, y, int64_t(1));
                        prop = prop or (t > int64_t(0));
                    }
                }
            } while (prop);

            recalculate_p(total_delta_p);

            UT += 2;
            prop = false;
            for (int x = 0; x < N; ++x) {
                for (int y = 0; y < K; ++y) {
                    if (field[x][y] != '#' && last_use[x][y] != UT) {
                        if (random01<VType>() < move_prob(x, y)) {
                            prop = true;
                            propagate_move(x, y, true);
                        } else {
                            propagate_stop(x, y, true);
                        }
                    }
                }
            }

            if (prop) {
                std::cout << "Tick " << i << ":\n";
                for (int j = 0; j < N; j++) {
                    for (int k = 0; k < K; k++) {
                        std::cout << field[j][k];
                    }
                    std::cout << "\n";
                }
                std::cout.flush();
            }
        }
    };

}