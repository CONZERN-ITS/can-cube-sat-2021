
set(_src_dir ${CMAKE_CURRENT_LIST_DIR})

add_library(jsmin INTERFACE)

target_include_directories(jsmin INTERFACE ${_src_dir}/inc)
