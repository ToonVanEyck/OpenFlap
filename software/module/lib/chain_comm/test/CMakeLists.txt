add_executable(${PROJECT_NAME}_test 
    test.c 
    test_uart.c 
    chain_comm_master_test.c 
    chain_comm_node_test.c 
    test_properties.c
)

target_include_directories(${PROJECT_NAME}_test
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(${PROJECT_NAME}_test 
  PRIVATE ${PROJECT_NAME}_node ${PROJECT_NAME}_master unity
)

add_test(${PROJECT_NAME}_test ${PROJECT_NAME}_test)

append_coverage_compiler_flags_to_target(${PROJECT_NAME}_node)
append_coverage_compiler_flags_to_target(${PROJECT_NAME}_master)
append_coverage_compiler_flags_to_target(${PROJECT_NAME}_shared)
add_dependencies(coverage ${PROJECT_NAME}_test)