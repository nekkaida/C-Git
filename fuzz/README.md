# Fuzz Testing for C-Git

This directory contains fuzz testing harnesses for security testing using libFuzzer.

## Prerequisites

- Clang compiler with libFuzzer support (Clang 6.0+)
- AddressSanitizer (included with Clang)

## Building Fuzz Targets

### SHA-1 Fuzzer
```bash
cd fuzz
clang -fsanitize=fuzzer,address -g -O1 \
    -I../include \
    -o fuzz_sha1 fuzz_sha1.c \
    ../src/core/sha1.c ../src/utils/error.c
```

### Object Fuzzer
```bash
clang -fsanitize=fuzzer,address -g -O1 \
    -I../include \
    -o fuzz_object fuzz_object.c \
    ../src/core/sha1.c ../src/core/object.c \
    ../src/utils/error.c ../src/utils/validation.c \
    -lz
```

### Validation Fuzzer
```bash
clang -fsanitize=fuzzer,address -g -O1 \
    -I../include \
    -o fuzz_validation fuzz_validation.c \
    ../src/utils/validation.c ../src/utils/error.c
```

## Running Fuzzers

### Basic Run (1000 iterations)
```bash
./fuzz_sha1 -runs=1000
```

### Continuous Fuzzing (until crash or Ctrl+C)
```bash
./fuzz_sha1
```

### With Corpus Directory
```bash
mkdir -p corpus_sha1
./fuzz_sha1 corpus_sha1/
```

### Timed Run (5 minutes)
```bash
./fuzz_sha1 -max_total_time=300
```

### With Multiple Jobs
```bash
./fuzz_sha1 -jobs=4 -workers=4
```

## Interpreting Results

- **BINGO**: A crash was found! The crashing input is saved in the current directory.
- **NEW**: A new code path was discovered and added to the corpus.
- **cov**: Current coverage statistics.

## Crash Analysis

When a crash is found:

1. The crashing input is saved as `crash-<hash>`
2. Reproduce with: `./fuzz_sha1 crash-<hash>`
3. Use GDB for debugging: `gdb --args ./fuzz_sha1 crash-<hash>`

## Coverage-Guided Tips

1. **Seed Corpus**: Provide valid examples to start:
   ```bash
   echo -n "abc" > corpus_sha1/seed1
   ./fuzz_sha1 corpus_sha1/
   ```

2. **Dictionary**: Create domain-specific tokens:
   ```bash
   echo '"blob"' > dict.txt
   echo '"tree"' >> dict.txt
   echo '"commit"' >> dict.txt
   ./fuzz_object -dict=dict.txt
   ```

## Security Targets

Priority targets for fuzzing:
1. **SHA-1 parsing**: Hex string validation
2. **Object decompression**: Malformed zlib data
3. **Path validation**: Directory traversal attempts
4. **Tree parsing**: Malformed tree entries
