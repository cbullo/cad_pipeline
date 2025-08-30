# Installation
## Prerequisites
I have developed it on Linux, I didn't have a chance to test it on MacOS. In principle, there shouldn't be issues there, but I can't guarantee that.
- Bazel 7.5.0 (note: there's a compatibility issue with Bazel 8.x)
## Steps
- Clone the repository
- Run this command from the root of the repo:
    `bazel run //frontend:serve`
- In a web browser, open this url:
    `http://localhost:8000/frontend/`
## Unit tests
So far I have only implemented native tests, no wasm testing yet.
- Run `bazel test //primes:primes_test`
## Benchmark
So far I have only implemented native benchmark, no wasm benchmark yet.
- Run `bazel run //primes:primes_benchmark`
