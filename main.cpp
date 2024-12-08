#include "field.h"

#include <bits/stdc++.h>

#include <utility>
#include <vector>

using namespace std;
using namespace Emulator;

#ifndef TYPES
#error "Not defined any types"
#endif

#ifndef SIZES
#define SIZES
#endif

#define FLOAT 1
#define DOUBLE 2
#define FIXED(n, k) (n*1000 + k)
#define FAST_FIXED(n, k) (n*100000 + k)
#define S(n, k) pair<int, int>(n, k)

/// Превращает int в тип, от которого он образован
/// \tparam n Число, которым закодировать тип
template<int n>
struct get_type_impl {
    using type = conditional_t<n == 1, float,
            conditional_t<n == 2, double,
                    conditional_t<(n > 100000), Fixed<n / 100000, n % 100000, true>,
                            conditional_t<(n > 1000), Fixed<n / 1000, n % 1000>, void>>>>;
};

/// Alias для get_type_impl<n>::type
/// \tparam n Число, которым закодировать тип
template<int n>
using get_type = get_type_impl<n>::type;

/// В compile-time заполняет массив из ВСЕХ ВОЗМОЖНЫХ комбинации 3-х типов даннных и размеров поля(включая "динамический")
/// \return Массив из пятерок int: первыe 3 - типы данных, последние 2 - размеры поля
constexpr auto get_fields() {
    constexpr pair<int, int> temp[] = {{ -1, -1 }, SIZES};
    constexpr int sz = sizeof(temp) / sizeof(pair<int, int>);

    constexpr auto types = array{TYPES};
    constexpr array<std::pair<int, int>, sz> sizes = {pair<int, int>(-1, -1), SIZES};

    array<tuple<int, int, int, int, int>, types.size() * types.size() * types.size() * sz> res{};

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

/// Массив ВСЕХ ВОЗМОЖНЫХ полей+типов
constexpr auto fields = get_fields();

/// Массив указателей на функции, порождающие соответствующие поля из `fields`
array<shared_ptr<AbstractField> (*)(), fields.size()> fields_generator;

/// Рекурисвный шаблон, создающий функцию для генерации поля для всех элементов массива `fields` с номерами 0...idx
/// \tparam idx Индекс максимального элемента для добавления
template<int idx>
struct FieldGeneratorIndex {
    FieldGeneratorIndex<idx - 1> x;

    FieldGeneratorIndex() {
        fields_generator[idx - 1] = generate;
    }

    static shared_ptr<AbstractField> generate() {
        return make_shared<FieldEmulator<
                get_type<get<0>(fields[idx - 1])>,
                get_type<get<1>(fields[idx - 1])>,
                get_type<get<2>(fields[idx - 1])>,
                get<3>(fields[idx - 1]),
                get<4>(fields[idx - 1])>>();
    }
};

template<>
struct FieldGeneratorIndex<0> {
    FieldGeneratorIndex() = default;
};

/// Форсирование заполнения массива `fields_generator`
[[maybe_unused]] FieldGeneratorIndex<fields.size()> generator{};

/// Считывает поле из файла
/// \param path путь к файлу
/// \return Вектор с элементами поля и его размеры
tuple<vector<vector<char>>, int, int> read_field(const char *path) {
    ifstream in(path);
    vector<vector<char>> ans;
    string s;
    while (not in.eof()) {
        getline(in, s);
        ans.emplace_back(s.begin(), s.end() - not in.eof());
    }
    return {ans, ans.size(), ans[0].size()};
}

int main() {
    shared_ptr<AbstractField> field;
    int type_p, type_v, type_vf;

    constexpr int T = 1'000'000;

    cin >> type_p >> type_v >> type_vf;

    auto [char_field, N, K] = read_field("../field1.txt");

    auto ind = find(fields.begin(), fields.end(), tuple(type_p, type_v, type_vf, N, K)) - fields.begin();
    if (ind != fields.size()) {
        field = fields_generator[ind]();
    } else {
        ind = find(fields.begin(), fields.end(), tuple(type_p, type_v, type_vf, -1, -1)) - fields.begin();
        if (ind == fields.size()) {
            cout << "Error: Unknown data types" << endl;
            exit(-1);
        }
        field = fields_generator[ind]();
    }

    field->init(char_field, N, K);
    for (int i = 0; i < T; ++i) {
        field->next(i);
    }
}