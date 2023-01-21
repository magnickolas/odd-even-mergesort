# Odd-even mergesort

A non-recursive implementation of [Batcher's odd-even mergesort][mergesort] for GPU.
The article about it is in my [blog][article].

## Running from source

```console
mkdir build && cd build
cmake ..
cmake --build .
./batcher_sort -h
```

Dependencies:

- Vulkan
- glslc from https://github.com/google/shaderc
- xxd for embedding the compiled shader

## Notes

Should only be used for educational purposes.

[article]: https://oplachko.com/2023-01/parallel-sorting-on-gpu-part-2/
[mergesort]: https://en.wikipedia.org/wiki/Batcher_odd%E2%80%93even_mergesort
