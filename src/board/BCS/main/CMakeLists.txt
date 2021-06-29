# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c" "init_helper.c" "input_op.c" "task_time_stats.c")
set(COMPONENT_ADD_INCLUDEDIRS "./Inc")

register_component()
