idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")

# Check if QEMU is defined and add the define to the compiler flags
if(QEMU)
add_definitions(-DQEMU=1)
endif()