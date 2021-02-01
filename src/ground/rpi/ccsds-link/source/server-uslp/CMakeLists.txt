
set(_src_dir ${CMAKE_CURRENT_LIST_DIR}/src)


add_executable(its-ccsds-link
	${_src_dir}/main.cpp
)


target_link_libraries(its-ccsds-link
PRIVATE
	ccsds-uslp-cpp
	sx126x
)

