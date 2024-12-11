#include "field.h"
#include "fields_factory.h"
#include "argument.h"

#include <vector>
#include <string>
#include <fstream>
#include <csignal>

std::tuple<int, int, int> read_field_params(const std::string &path) {
    std::ifstream in(path);
    int N, K, T;
    in >> N >> K >> T;
    return {N, K, T};
}

bool save_flag = false;

void on_interrupt(int _) {
    save_flag = true;
}

int main(int argc, char **argv) {
    signal(SIGINT, on_interrupt);

    ArgumentParser args(argc, argv);

    int type_p = Emulator::TypeEncoder::get_type(args.get_option("--p-type"));
    int type_v = Emulator::TypeEncoder::get_type(args.get_option("--v-type"));
    int type_vf = Emulator::TypeEncoder::get_type(args.get_option("--v-flow-type"));
    std::string filename = args.get_option("--field");
    std::string save_filename = args.get_option("--save-field");

    int T = 1'000'000;

    auto [N, K, t] = read_field_params(filename);

    auto field = get_field(type_p, type_v, type_vf, N, K);

    field->load(filename);
    for (int i = t; i < T; ++i) {
        if (save_flag) {
            std::cout << "Saving field to `" << save_filename << "`" << std::endl;

            field->save(save_filename, i);

            std::cout << "Complete. Enter any number to continue: ";

            save_flag = false;
            int tmp;
            std::cin >> tmp;

            std::cout << std::endl;
        }
        field->next(i);
    }
}