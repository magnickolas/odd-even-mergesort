#include "batcher_sort.h"
#include "opts.h"
#include "timer.h"

int main(int argc, char* argv[]) {
    auto opts = Options::parse(argc, argv);

    auto guard = VkInfoGuard{};
    auto info = guard.get();

    // prepare an array storage
    info->arr = create_array_storage(opts.n, info);
    info->arr.fill_random(opts.seed);

    // Create a copy of the array for CPU to sort for
    // benchmark comparison and verifying correctness
    std::vector<CTYPE> arr_cpu(opts.n);
    std::copy_n(info->arr.get_buffer(), opts.n, arr_cpu.data());

    if (opts.debug)
        info->arr.debug_print(opts.n);
    Timer{"GPU time difference: "}.run([&] { //
        sort_vec(info);
    });
    if (opts.debug)
        info->arr.debug_print(opts.n);

    Timer{"CPU time difference: "}.run([&] { //
        std::sort(std::begin(arr_cpu), std::end(arr_cpu));
    });

    if (!info->arr.compare_with_reference(arr_cpu)) {
        throw std::runtime_error("GPU and CPU results differ");
    }
    return 0;
}
