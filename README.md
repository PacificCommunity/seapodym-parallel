![Build and test](https://github.com/PacificCommunity/seapodym-parallel/actions/workflows/build.yml/badge.svg)

# seapodym-parallel
Code development of population dynamics models aka SEAPODYM with parallel runs and parameter estimations

## Prerequisites

You will need:
 * A C++ compiler (e.g. g++) and MPI libraries installed (e.g. OpenMPI)
 * CMake
 * ADMB


## How to build the seapodym-parallel

```
git clone  git@github.com:PacificCommunity/seapodym-parallel
cd seapodym-parallel
mkdir build
cd build
cmake -DADMB_HOME=path ..
cmake --build .
```

## How to build the documention

In the build directory
```
cmake --build . --target doc_doxygen
```
This will generate the documention in the top source directory `docs`.

## How to test the code

In the build directory
```
ctest
```
