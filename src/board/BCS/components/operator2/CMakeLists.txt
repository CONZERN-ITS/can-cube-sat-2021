idf_component_register(SRCS "./Src/operator2.c"
                       INCLUDE_DIRS "./Inc"
                       PRIV_REQUIRES mavlink_comp router its_config log_collector shift_reg msp)