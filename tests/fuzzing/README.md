Install with

```
CC=/usr/bin/clang CXX=/usr/bin/clang++
CFLAGS="-O1 -g -fno-omit-frame-pointer -fno-lto -fsanitize=address,fuzzer-no-link"
CXXFLAGS="-O1 -g -fno-omit-frame-pointer -fno-lto -fsanitize=address,fuzzer-no-link"
LDFLAGS="-fno-lto -fsanitize=address,fuzzer-no-link"
CMAKE_ARGS="-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF" pip install .
```
Run with

```
LD_PRELOAD="$(python -c
 "import atheris; print(atheris.path())")/asan_with_fuzzer.so" python 
tests/fuzzing/smry_fuzz_target.py -detect_leaks=0 -max_len=22000000 -r
ss_limit_mb=32000000000 tests/fuzzing/smry_corpus/
```

Its beneficial to also use

```
export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer-18
```

to get debug symbols.
