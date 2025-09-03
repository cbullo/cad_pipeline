# Installation
## Prerequisites
I have developed it on Linux (WSL), I didn't have a chance to test it on other systems. In principle, there shouldn't be issues there, but I can't guarantee that.
- Bazel 8.3.1 (that's the version I used)
- Modern C++ compiler (looks like the default GH Action one is too old, as it doesn't support <print>)
## Steps
- Clone the repository
- Run this command from the root of the repo to run generate an stl file with the result:
    `bazel run //cad_pipeline:cad_binary`
- Or run this command to see a vizualization in a browser:
    `bazel run //frontend:serve`
