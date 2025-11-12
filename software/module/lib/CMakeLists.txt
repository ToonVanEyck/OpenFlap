add_subdirectory(interpolation)
add_subdirectory(rbuff)

if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "ARM")

    add_library(openflap INTERFACE)

    add_subdirectory(rtt_utils)
    add_subdirectory(simple_term)
    add_subdirectory(flash)
    add_subdirectory(uart_driver)
    add_subdirectory(pid)
    add_subdirectory(madelink)
    add_subdirectory(puya_libs)

    target_link_libraries(openflap INTERFACE
        interpolation
        rbuff
        rtt_utils
        simple_term
        flash
        uart_driver
        pid
        puya_ll
        madelink_node
    )
endif()

