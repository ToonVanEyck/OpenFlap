set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Use native compilers instead of cross-compilation toolchain
set(CMAKE_C_COMPILER gcc)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_AR gcc-ar)
set(CMAKE_RANLIB gcc-ranlib)

# Remove ARM-specific flags and use native compilation flags
set(OBJECT_GEN_FLAGS "")
set(OPTIMALIZATION_FLAGS "")

set(CMAKE_C_FLAGS   "${OBJECT_GEN_FLAGS} ${OPTIMALIZATION_FLAGS} -std=gnu99 -Wall -ffunction-sections -fdata-sections" CACHE INTERNAL "C Compiler options")
set(CMAKE_CXX_FLAGS "${OBJECT_GEN_FLAGS} ${OPTIMALIZATION_FLAGS} -std=c++11 " CACHE INTERNAL "C++ Compiler options")
set(CMAKE_ASM_FLAGS "${OBJECT_GEN_FLAGS} ${OPTIMALIZATION_FLAGS} -Wa,--warn" CACHE INTERNAL "ASM Compiler options")


# Default C compiler flags
set(CMAKE_C_FLAGS_DEBUG_INIT "-g3 -Og -gdwarf-3 -Wall -pedantic -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG_INIT}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE_INIT "-O3 -Wall")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE_INIT}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL_INIT "-Os -Wall")
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL_INIT}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "-O2 -g -Wall")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO_INIT}" CACHE STRING "" FORCE)
# Default C++ compiler flags
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g3 -Og -gdwarf-3 -Wall -pedantic -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG_INIT}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3 -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE_INIT}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "-Os -Wall")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL_INIT}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "-O2 -g -Wall")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT}" CACHE STRING "" FORCE)


# Remove ARM-specific linker flags
set(CMAKE_EXE_LINKER_FLAGS "-static -lc -lm -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections -Wl,--print-memory-usage" CACHE INTERNAL "Linker options")
# set(CMAKE_EXE_LINKER_FLAGS "-static -lc -lm -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections -Wl,--print-memory-usage -T ${CMAKE_SOURCE_DIR}/puya_libs/LDScripts/${PUYA_CHIP}.ld" CACHE INTERNAL "Linker options")
# set(CMAKE_EXE_LINKER_FLAGS "-u _printf_float ${CMAKE_EXE_LINKER_FLAGS}" CACHE INTERNAL "Linker options") # Enable float support for printf

# Use native objcopy and size utilities
set(CMAKE_OBJCOPY objcopy CACHE INTERNAL "objcopy tool")
set(CMAKE_SIZE_UTIL size CACHE INTERNAL "size tool")

# Remove cross-compilation find root path settings
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
