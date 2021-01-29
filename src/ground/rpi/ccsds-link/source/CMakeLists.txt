cmake_minimum_required(VERSION 3.17)


project(its-ccsds-link
LANGUAGES
	C CXX
)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(ccsds-uslp-cpp)
add_subdirectory(sx126x)
add_subdirectory(application)