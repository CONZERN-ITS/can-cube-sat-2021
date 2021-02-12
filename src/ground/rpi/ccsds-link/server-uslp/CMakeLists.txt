cmake_minimum_required(VERSION 3.16)


project(its-server-uslp
LANGUAGES
	CXX
)

add_subdirectory(libs/ccsds-uslp-cpp)


set(_src_dir ${CMAKE_CURRENT_LIST_DIR}/src)

add_executable(server-uslp
	${_src_dir}/main.cpp
	${_src_dir}/json.hpp
	${_src_dir}/event_handler.hpp
)


target_link_libraries(server-uslp
PRIVATE
	ccsds-uslp-cpp
	zmq
)
