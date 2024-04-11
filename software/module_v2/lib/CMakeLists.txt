add_subdirectory(debug_io)

add_library(${LIB_NAME} STATIC
    $<TARGET_OBJECTS:debug_io>
)

get_property(object_include_dirs TARGET debug_io PROPERTY INCLUDE_DIRECTORIES)
get_property(object_link_libs TARGET debug_io PROPERTY LINK_LIBRARIES)

# target_link_libraries(${LIB_NAME} PUBLIC $<TARGET_OBJECTS:debug_io>)
# target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${LIB_NAME} PUBLIC ${object_include_dirs})
target_link_libraries(${LIB_NAME} PUBLIC ${object_link_libs})