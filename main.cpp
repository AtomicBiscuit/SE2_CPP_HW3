#include "field.h"
#include "fields_factory.h"
#include "argument.h"

#include <vector>
#include <string>
#include <fstream>

std::tuple<std::vector<std::vector<char>>, int, int> read_field(const char *path) {
    std::ifstream in(path);
    std::vector<std::vector<char>> ans;
    std::string s;
    while (not in.eof()) {
        getline(in, s);
        ans.emplace_back(s.begin(), s.end() - not in.eof());
    }
    return {ans, ans.size(), ans[0].size()};
}

int main(int argc, char **argv) {
    ArgumentParser args(argc, argv);

    int type_p = Emulator::TypeEncoder::get_type(args.get_option("--p-type"));
    int type_v = Emulator::TypeEncoder::get_type(args.get_option("--v-type"));
    int type_vf = Emulator::TypeEncoder::get_type(args.get_option("--v-flow-type"));

    int T = 1'000'000;

    auto [char_field, N, K] = read_field("../field1.txt");

    auto field = get_field(type_p, type_v, type_vf, N, K);

    field->init(char_field, N, K);
    for (int i = 0; i < T; ++i) {
        field->next(i);
    }
}