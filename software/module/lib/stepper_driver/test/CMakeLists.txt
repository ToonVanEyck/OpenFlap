
add_executable(test_stepper_driver test_stepper_driver.c ../stepper_driver.c)
target_include_directories(test_stepper_driver PRIVATE ../inc)
target_link_libraries(test_stepper_driver PRIVATE unity)
add_test(NAME test_stepper_driver COMMAND test_stepper_driver)