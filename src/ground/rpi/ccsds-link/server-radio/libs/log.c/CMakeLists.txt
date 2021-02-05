
set(_src_dir ${CMAKE_CURRENT_LIST_DIR})

add_library(logc STATIC
	${_src_dir}/src/log.c
	${_src_dir}/inc/log.h
)

target_compile_definitions(logc PUBLIC LOG_USE_COLOR)

target_include_directories(logc
PUBLIC
	${_src_dir}/inc
)
