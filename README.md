# Installation
## Prerequisites
I have developed it on Linux (WSL), I didn't have a chance to test it on other systems. In principle, there shouldn't be issues there, but I can't guarantee that.
- Bazel 8.3.1
- C++ compiler
## Steps
- Clone the repository
- Run this command from the root of the repo:
    `bazel run //frontend:serve`
## Unit tests
So far I have only implemented native tests, no wasm testing yet.
- Run `bazel test //primes:primes_test`
## Benchmark
So far I have only implemented native benchmark, no wasm benchmark yet.
- Run `bazel run //primes:primes_benchmark`
