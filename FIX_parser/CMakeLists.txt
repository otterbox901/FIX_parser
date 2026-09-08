cmake_minimum_required(VERSION 3.16)
project(FIX_parser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(FIX_parser
        src/main.cpp
        include/decoder_versions/decoder_v1+v2.h
        include/decoder_versions/decoder.hpp
        include/output_components/44tag_array.hpp
)
target_include_directories(FIX_parser PRIVATE include)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(FIX_parser PRIVATE -O3)
    set_target_properties(FIX_parser PROPERTIES STRIP ON)
endif()

install(TARGETS FIX_parser RUNTIME DESTINATION bin)