#pragma once

#include <map>
#include <string>
#include <iostream>

class ArgumentParser {
    std::map<std::string, std::string> args_;
public:
    ArgumentParser(int argc, char **argv) {
        while (--argc > 0) {
            size_t pos = std::string(argv[argc]).find_first_of('=');
            if (pos == std::string::npos) {
                std::cout << "Error: expected value of " << std::string(argv[argc]) << std::endl;
                exit(-1);
            }
            args_[std::string(argv[argc], pos)] = std::string(argv[argc] + pos + 1, strlen(argv[argc]) - pos - 1);
        }
    }

    std::string get_option(const std::string &target) {
        if (not args_.contains(target)) {
            std::cout << "Error: expected option " << target << std::endl;
            exit(-1);
        }
        return args_[target];
    }
};