#include "opts.h"
#include "cxxopts.h"
#include <cstdint>

Options Options::parse(int argc, char** argv) {
    cxxopts::Options options("Batcher Sort", "Sort an array of integers");
    options.add_options()("n", "Array length",
                          cxxopts::value<uint32_t>()) //
        ("s,seed", "Random seed",
         cxxopts::value<uint32_t>()->default_value("0")) //
        ("d,debug", "Print initial and sorted array")    //
        ("h,help", "Print usage");
    options.parse_positional("n");
    auto result = options.parse(argc, argv);
    Options opts;
    opts.n = result["n"].as<uint32_t>();
    opts.seed = result["seed"].as<uint32_t>();
    opts.debug = result["debug"].as<bool>();
    opts.help = result["help"].as<bool>();
    if (opts.help) {
        std::cout << options.help() << std::endl;
        exit(0);
    }
    return opts;
};