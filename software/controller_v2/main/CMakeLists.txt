idf_component_register(
    SRCS 
        "main.c"
        "utils.c" 
    INCLUDE_DIRS 
        "include"
)

# Check if QEMU is defined and add the define to the compiler flags
if(QEMU)
add_definitions(-DQEMU=1)
endif()