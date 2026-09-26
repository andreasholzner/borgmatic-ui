# borgmatic-ui

A Qt desktop front end for [borgmatic](https://torsion.org/borgmatic/).

## Build

### Requirements

- CMake ≥ 3.16 and a C++20 compiler (GCC or Clang)
- Qt 6 (Core, Widgets, Concurrent, Test)
- Boost ≥ 1.88 including the compiled Boost.Process library
- cereal, spdlog, nlohmann_json
- Catch2 v3 (tests only)
- trompeloeil (tests only; downloaded automatically if not installed)
- borgmatic at runtime, found on `PATH` (falls back to `/usr/bin/borgmatic`)

On Arch/Manjaro:

```sh
sudo pacman -S cmake qt6-base boost cereal spdlog nlohmann-json catch2
```

On Debian/Ubuntu:

```sh
sudo apt install cmake g++ qt6-base-dev libboost-dev libboost-process-dev \
    libcereal-dev libspdlog-dev nlohmann-json3-dev catch2
```

### Compile

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable is `build/borgmatic-ui`.

To build without the tests, add `-DBUILD_TESTING=OFF` to the first command.

### Run the tests

```sh
ctest --test-dir build --output-on-failure
```
