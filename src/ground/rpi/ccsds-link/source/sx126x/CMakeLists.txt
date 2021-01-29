
set(_remote_src_dir ${CMAKE_CURRENT_LIST_DIR}/remote)
set(_local_src_dir ${CMAKE_CURRENT_LIST_DIR})


add_library(sx126x STATIC
	${_remote_src_dir}/inc/sx126x_api.h
	${_remote_src_dir}/inc/sx126x_board.h
	${_remote_src_dir}/inc/sx126x_defs.h
	${_remote_src_dir}/inc/sx126x_drv.h
	${_remote_src_dir}/inc/sx126x_events.h

	${_remote_src_dir}/src/sx126x_api.c
	${_remote_src_dir}/src/sx126x_drv.c

	${_local_src_dir}/inc/sx126x_board_rpi.h
	${_local_src_dir}/src/sx126x_board_rpi.c
)

target_include_directories(sx126x
PUBLIC
	${_remote_src_dir}/inc
	${_local_src_dir}/inc
)
