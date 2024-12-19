#pragma once

#include <type_traits>
#include <memory>
#include <utility>
#include <tuple>
#include <array>
#include <iostream>
#include <string>
#include <map>

#include "field.h"

#ifndef TYPES
#error "Not defined any types"
#endif

#ifndef SIZES
#define SIZES
#endif

#define FLOAT {"FLOAT", 1}
#define DOUBLE {"DOUBLE", 2}
#define FIXED(n, k) {"FIXED("#n","#k")", (n*1000 + k)}
#define FAST_FIXED(n, k) {"FAST_FIXED("#n","#k")", (n*100000 + k)}

namespace Emulator {
    class TypeEncoder {
        static const inline std::map<std::string, int> encoded_ = {TYPES};

    public:
        static int get_type(const std::string &name) {
            if (not encoded_.contains(name)) {
                std::cout << "Error: type `" << name << "` not found" << std::endl;
                exit(-1);
            }
            return encoded_.at(name);
        }
    };

}

#undef FLOAT
#undef DOUBLE
#undef FIXED
#undef FAST_FIXED

#define FLOAT 1
#define DOUBLE 2
#define FIXED(n, k) (n*1000 + k)
#define FAST_FIXED(n, k) (n*100000 + k)
#define S(n, k) std::pair<int, int>(n, k)

namespace Emulator {

    namespace details {

        /// Превращает int в тип, от которого он образован
        /// \tparam n Число, которым закодирован тип
        template<int n>
        struct get_type_impl {
            using type = std::conditional_t<n == 1, float,
                    std::conditional_t<n == 2, double,
                            std::conditional_t<(n > 100000), Fixed<n / 100000, n % 100000, true>,
                                    std::conditional_t<(n > 1000), Fixed<n / 1000, n % 1000, false>, void>>>>;
        };
    }

    /// Alias для get_type_impl<n>::type
    /// \tparam n Число, которым закодирован тип
    template<int n>
    using get_type = details::get_type_impl<n>::type;

    /// В compile-time заполняет массив из ВСЕХ ВОЗМОЖНЫХ комбинации 3-х типов данных и размеров поля(включая "динамический")
    /// \return Массив из пятерок int: первые 3 - типы данных, последние 2 - размеры поля
    constexpr auto get_fields() {
        constexpr std::pair<int, int> temp[] = {{-1, -1}, SIZES};
        constexpr int sz = sizeof(temp) / sizeof(std::pair<int, int>);

        constexpr auto types = std::array{TYPES};
        constexpr std::array<std::pair<int, int>, sz> sizes = {std::pair<int, int>(-1, -1), SIZES};

        std::array<std::tuple<int, int, int, int, int>, types.size() * types.size() * types.size() * sz> res{};

        int i = 0;
        for (int type_p: types) {
            for (int type_v: types) {
                for (int type_vf: types) {
                    for (auto field_size: sizes) {
                        res[i++] = {type_p, type_v, type_vf, field_size.first, field_size.second};
                    }
                }
            }
        }
        return res;
    }

    /// Массив ВСЕХ ВОЗМОЖНЫХ полей+типов`
    constexpr auto fields = get_fields();

    /// Массив указателей на функции, порождающие соответствующие поля из `fields`
    std::array<std::shared_ptr<AbstractField>(*)(), fields.size()> fields_generator;

    /// Рекурсивный шаблон, создающий функцию для генерации поля для всех элементов массива `fields` с номерами 0...idx
    /// \tparam idx Индекс максимального элемента для добавления
    template<int idx>
    struct FieldGeneratorIndex {
        FieldGeneratorIndex<idx - 1> x;

        FieldGeneratorIndex() {
            fields_generator[idx - 1] = generate;
        }

        static std::shared_ptr<AbstractField> generate() {
            return std::make_shared<FieldEmulator<
                    get_type<get<0>(fields[idx - 1])>,
                    get_type<get<1>(fields[idx - 1])>,
                    get_type<get<2>(fields[idx - 1])>,
                    get<3>(fields[idx - 1]),
                    get<4>(fields[idx - 1]) >>();
        }
    };

    template<>
    struct FieldGeneratorIndex<0> {
        FieldGeneratorIndex() = default;
    };

    /// Заполнения массива `fields_generator`
    [[maybe_unused]] FieldGeneratorIndex<fields.size()> generator{};
}

std::shared_ptr<Emulator::AbstractField> get_field(int type_p, int type_v, int type_vf, int N, int K) {
    using Emulator::fields;
    using Emulator::fields_generator;

    auto ind = std::find(fields.begin(), fields.end(), std::tuple(type_p, type_v, type_vf, N, K)) - fields.begin();

    std::shared_ptr<Emulator::AbstractField> field;
    if (ind != fields.size()) {
        field = fields_generator[ind]();
    } else {
        ind = std::find(fields.begin(), fields.end(), std::tuple(type_p, type_v, type_vf, -1, -1)) - fields.begin();
        if (ind == fields.size()) {
            std::cout << "Error: Unknown data types" << std::endl;
            exit(-1);
        }
        field = fields_generator[ind]();
    }

    return field;
}