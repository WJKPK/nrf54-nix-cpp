cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS ../zephyr)
project(nrf54l15_cpp_app LANGUAGES C CXX)

target_sources(app PRIVATE src/main.cpp)
