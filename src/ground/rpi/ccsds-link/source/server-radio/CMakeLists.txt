
set(_src_dir ${CMAKE_CURRENT_LIST_DIR}/src)


add_executable(server-radio
	${_src_dir}/main.c
)


target_link_libraries(server-radio
PRIVATE
	sx126x
	gpiod
)

