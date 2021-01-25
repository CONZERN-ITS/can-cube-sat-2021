cmake_minimum_required(VERSION 3.17)

project(ccsds-uslp-cpp-dev)

set (CMAKE_CXX_STANDARD 17)

add_subdirectory(../../common/ccsds/uslp-cpp common/ccsds/uslp-cpp)

add_executable(ccsds-uslp-cpp-dev
	src/main.cpp
	src/bytes_printer.hpp
	src/lorem_ipsum.hpp
)

target_link_libraries(ccsds-uslp-cpp-dev PRIVATE ccsds-uslp-cpp)