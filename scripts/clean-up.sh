#!/bin/bash

echo "Clean up CMake files ... "
rm -rf detect_cuda_version.cc detect_cuda_compute_capabilities.cpp CMakeCache.txt CMakeFiles Games/CMakeFiles MiniZero/CMakeFiles py/CMakeFiles cmake_install.cmake Games/cmake_install.cmake MiniZero/cmake_install.cmake py/cmake_install.cmake
echo "Clean up Result files ... "
rm -rf Debug Release
echo "Clean Make file ... "
rm -f Makefile Games/Makefile MiniZero/Makefile py/Makefile
