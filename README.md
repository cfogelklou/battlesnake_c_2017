# battlesnake_c_2017 Implement Battlesnake in C #

## Example template code ##

This code is be used with [battlesnake 2017 API](https://github.com/sendwithus/battlesnake), enabling battlesnake logic to be written in "C".

The underlying messaging layers are written in C++ to enable use of json.hpp for parsing and also just because the way we got this working wasn't as important as the getting it working.

The [nlohmann::json](https://github.com/nlohmann/json) library used for parsing incoming packets requires full C++11 support, and will only work with **fully-updated** MSVC2015 (not release 1), MSVC2017, GCC4.9+, Newer Clang, etc.

TCP support is hacked together using sockets and some http "cheating" but IS cross-platform and works on Windows, Linux, and OSX, with some small **#ifdef** to allow stuff to work on both **nix** and **windows** platforms.

## Build Instructions ##

  git clone https://github.com/cfogelklou/battlesnake_c_2017 --recursive
  mkdir build
  cd build
  cmake ..
  make
  ./battlesnake_c
