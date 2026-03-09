cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../lib)

find_package(Zephyr REQUIRED HINTS ../zephyr)
project(nrf54l15_cpp_app LANGUAGES C CXX)

# Disable exceptions and RTTI for embedded/ETL usage
add_compile_options(-fno-exceptions -fno-rtti)
zephyr_cc_option(-Wfatal-errors)
add_compile_options(-Wno-vla-cxx-extension)

target_sources(app PRIVATE src/main.cpp)

zephyr_include_directories(${ZEPHYR_BASE}/../etl/include)
