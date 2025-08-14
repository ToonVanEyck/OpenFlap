add_executable(${PROJECT_NAME}_test test.c)

target_link_libraries(${PROJECT_NAME}_test 
  PRIVATE ${PROJECT_NAME} unity
)

add_test(${PROJECT_NAME}_test ${PROJECT_NAME}_test)

append_coverage_compiler_flags_to_target(${PROJECT_NAME})
add_dependencies(coverage ${PROJECT_NAME}_test)