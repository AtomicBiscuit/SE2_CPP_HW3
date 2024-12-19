#include "include/fields_factory.h"
#include "include/argument.h"

#include <string>
#include <fstream>

std::tuple<int, int, int> read_field_params(const std::string &path) {
    std::ifstream in(path);
    int N, K, T;
    in >> N >> K >> T;
    return {N, K, T};
}

int main(int argc, char **argv) {
    ArgumentParser args(argc, argv);

    int type_p = Emulator::TypeEncoder::get_type(args.get_option("--p-type"));
    int type_v = Emulator::TypeEncoder::get_type(args.get_option("--v-type"));
    int type_vf = Emulator::TypeEncoder::get_type(args.get_option("--v-flow-type"));

    std::string filename = args.get_option("--field");

    int workers = std::stoi(args.get_option("--threads-count"));

    int T = 1'000'000;

    auto [N, K, t] = read_field_params(filename);

    auto field = get_field(type_p, type_v, type_vf, N, K);

    field->load(filename);
    field->init_workers(workers);

    auto timer = std::chrono::steady_clock::now();
    for (int i = 0; i < T; ++i) {
        field->next(i);
        if (i == 10'000) {
            break;
        }
    }
    std::cout << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - timer).count()
              << std::endl;
}