cmake_minimum_required(VERSION 3.16)


project(its-server-radio
LANGUAGES
	C
)

set(CMAKE_CXX_STANDARD 17)
set(_src_dir ${CMAKE_CURRENT_LIST_DIR}/src)

add_executable(server-radio
	${_src_dir}/main.c
	${_src_dir}/network.h
	${_src_dir}/network.c
	${_src_dir}/radio.h
	${_src_dir}/radio.c
)


add_subdirectory(libs/sx126x)

target_link_libraries(server-radio
PRIVATE
	sx126x
	gpiod
	zmq
)

