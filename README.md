![Build and test](https://github.com/PacificCommunity/seapodym-parallel/actions/workflows/build.yml/badge.svg)

# seapodym-parallel
Code development of population dynamics models aka SEAPODYM with parallel runs and parameter estimations

## Prerequisites

You will need:
 * A C++ compiler (e.g. g++) and MPI libraries installed (e.g. OpenMPI)
 * CMake
 * ADMB 13.1 (13.2 will not work)
 * [spdlog](https://github.com/gabime/spdlog). On Ubuntu: `apt install libspdlog-dev`.
 * libfmt. On Ubuntu: `apt install libfmt-dev`


## How to build the seapodym-parallel

```
git clone  git@github.com:PacificCommunity/seapodym-parallel
cd seapodym-parallel
mkdir build
cd build
cmake -DADMB_HOME=<path to admb> -D CMAKE_INSTALL_PREFIX=<install dir> ..
cmake --build .
cmake --install .
```

## How to build the documention

In the build directory
```
cmake --build . --target doc_doxygen
```
This will generate the documention in the top source directory `docs`. 

## Documentation

Documentation is built and pushed after every code change to [here](https://pacificcommunity.github.io/seapodym-parallel/).

## How to test the code

In the build directory
```
ctest
```
