#!/bin/bash
rm CMakeCache.txt
rm Makefile
rm cmake_install.cmake
rm -r CMakeFiles
rm ./src/Makefile
rm ./src/cmake_install.cmake
rm -r ./src/CMakeFiles

cmake .
make