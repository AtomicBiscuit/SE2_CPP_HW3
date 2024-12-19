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
#include <fstream>
#include <atomic>
#include <tuple>
#include <stack>

#include "numbers.h"
#include "static_array.h"
#include "utilities.h"
#include "vector_field.h"
#include "tasks.h"
#include "workers.h"


namespace Emulator {
    struct AbstractField {
        virtual void next(int) = 0;

        virtual void load(const std::string &) = 0;

        virtual ~AbstractField() = default;

        virtual void init_workers(int) = 0;
    };

    template<typename PType, typename VType, typename VFType, int N_val, int K_val>
    class FieldEmulator : public AbstractField {
        using p_type = PType;
        using v_type = VType;
        using vf_type = VFType;
        using full_type = FieldEmulator<PType, VType, VFType, N_val, K_val>;

        int N = 0;
        int K = 0;

        Array<char, N_val, K_val> field{};

        VectorField<VType, N_val, K_val> velocity = {};
        VectorField<VFType, N_val, K_val> velocity_flow = {};

        Array<int64_t, N_val, K_val> dirs{};
        Array<int64_t, N_val, K_val> last_use{};
        int UT = 0;
        int last_active = 0;

        PType rho[256];
        Array<PType, N_val, K_val> p{}, old_p{};

        Array<std::mutex, N_val, K_val> p_mutex{};

        std::vector<std::unique_ptr<Task>> g_tasks;
        std::vector<std::unique_ptr<Task>> p_tasks;
        std::vector<std::unique_ptr<Task>> recalc_p_tasks;
        std::vector<std::unique_ptr<Task>> output_field_task;

        WorkerHandler main_handler{};
        WorkerHandler output_handler{};
    public:
        constexpr FieldEmulator() = default;

        void next(int i) override {
            apply_external_forces();

            apply_p_forces();

            apply_forces_on_flow();

            recalculate_p();

            output_handler.wait_until_end();

            bool prop = apply_move_on_flow();

            if (prop) {
                last_active = i;
                output_handler.set_tasks(&output_field_task);
            }
        }

        void load(const std::string &filename) override {
            std::ifstream file(filename);
            int t;
            if (not file.is_open()) {
                std::cout << "Error: can`t open file `" << filename << "`" << std::endl;
            }
            file >> N >> K >> t >> UT;
            load_array(field, file, N, K);
            init();
        }

        void init_workers(int n) override {
            if (n < 1) {
                throw std::runtime_error("Must be at least 1 thread");
            }
            main_handler.init(n);
            output_handler.init(1);
        }

        friend class ApplyGTask<full_type>;

        friend class ApplyPTask<full_type>;

        friend class RecalcPTask<full_type>;

        friend class OutFieldTask<full_type>;

    private:
        void update_p(int x, int y, const PType &val) {
            std::lock_guard lock(p_mutex[x][y]);
            p[x][y] += val;
        }

        void init() {
            velocity.v.init(N, K);
            last_use.init(N, K);
            velocity_flow.v.init(N, K);
            dirs.init(N, K);

            p.init(N, K);
            old_p.init(N, K);
            p_mutex.init(N, K);

            g_tasks.reserve(N);
            p_tasks.reserve(N);
            recalc_p_tasks.reserve(N);
            for (int i = 0; i < N; i++) {
                g_tasks.push_back(std::make_unique<ApplyGTask<full_type>>(i, *this));
                p_tasks.push_back(std::make_unique<ApplyPTask<full_type>>(i, *this));
                recalc_p_tasks.push_back(std::make_unique<RecalcPTask<full_type>>(i, *this));
            }

            output_field_task.push_back(std::make_unique<OutFieldTask<full_type>>(*this));

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
                if (field[nx][ny] == '#' || last_use[nx][ny] >= UT) {
                    continue;
                }
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
                VFType t;
                bool prop;
                std::pair<int, int> end;
                do {
                    std::tie(t, prop, end) = propagate_flow(nx, ny, vp);
                } while (end == std::pair(nx, ny));
                ret += t;
                if (prop) {
                    velocity_flow.add(x, y, dx, dy, t);
                    last_use[x][y] = UT;
                    return {t, end != std::pair(x, y), end};
                }
            }
            last_use[x][y] = UT;
            return {ret, false, {-1, -1}};
        }

        inline bool is_stoppable(int x, int y) {
            for (auto [dx, dy]: deltas) {
                int nx = x + dx, ny = y + dy;
                if (field[nx][ny] != '#' && last_use[nx][ny] < UT - 1 && velocity.get(x, y, dx, dy) > int64_t(0)) {
                    return false;
                }
            }
            return true;
        }

        void propagate_stop(int x_, int y_) {
            std::stack<std::pair<int, int>> nxt;
            nxt.emplace(x_, y_);
            last_use[x_][y_] = UT;
            while (not nxt.empty()) {
                auto [x, y] = nxt.top();
                nxt.pop();
                for (auto [dx, dy]: deltas) {
                    int nx = x + dx, ny = y + dy;
                    if (field[nx][ny] == '#' || last_use[nx][ny] == UT || velocity.get(x, y, dx, dy) > int64_t(0) ||
                        not is_stoppable(nx, ny)) {
                        continue;
                    }
                    last_use[nx][ny] = UT;
                    nxt.emplace(nx, ny);
                }
            }
        }

        VType move_probability(int x, int y) {
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
                ret = (last_use[nx][ny] == UT - 1 || propagate_move(nx, ny, false));
            } while (!ret);

            last_use[x][y] = UT;

            for (auto [dx, dy]: deltas) {
                int forward_x = x + dx, forward_y = y + dy;
                if (field[forward_x][forward_y] != '#' and last_use[forward_x][forward_y] < UT - 1 and
                    velocity.get(x, y, dx, dy) < int64_t(0) and is_stoppable(forward_x, forward_y)) {
                    propagate_stop(forward_x, forward_y);
                }
            }
            if (ret and !is_first) {
                swap(x, y, nx, ny);
            }
            return ret;
        }

        void apply_external_forces() {
            main_handler.set_tasks(&g_tasks);
            main_handler.wait_until_end();
        }

        void apply_p_forces() {
            old_p = p;
            main_handler.set_tasks(&p_tasks);
            main_handler.wait_until_end();
        }

        void apply_forces_on_flow() {
            velocity_flow.v.clear();
            int cnt = 0;
            bool prop;
            do {
                cnt++;
                UT += 2;
                prop = false;
                for (int x = 0; x < N; x++) {
                    for (int y = 0; y < K; y++) {
                        if (field[x][y] == '#' or last_use[x][y] == UT) {
                            continue;
                        }
                        auto [t, _unused1, _unused2] = propagate_flow(x, y, int64_t(1));
                        if (t > int64_t(0)) {
                            prop = true;
                            --y;
                        }
                    }
                }
            } while (prop);
        }

        void recalculate_p() {
            main_handler.set_tasks(&recalc_p_tasks);
            main_handler.wait_until_end();
        }

        bool apply_move_on_flow() {
            UT += 2;
            bool prop = false;
            for (int x = 0; x < N; ++x) {
                for (int y = 0; y < K; ++y) {
                    if (field[x][y] != '#' && last_use[x][y] != UT) {
                        if (random01<VType>() < move_probability(x, y)) {
                            prop = true;
                            propagate_move(x, y, true);
                        } else {
                            propagate_stop(x, y);
                        }
                    }
                }
            }
            return prop;
        }
    };
}
