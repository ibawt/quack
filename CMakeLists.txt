cmake_minimum_required(VERSION 3.0)
project(quack)

configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories("${gtest_SOURCE_DIR}/include")
endif()

include(GoogleTest)

enable_testing()

set (main src/main.c)

set(files
  src/symbol.c
  src/parser.c
  src/memory.c
  src/types.c
  src/compiler.c
  src/map.c
  src/env.c
  src/vector.c
  src/engine.c
  src/cps.c
  )

add_executable(quack ${main} ${files})
set_target_properties(quack PROPERTIES C_STANDARD 99)
target_link_libraries(quack readline)
target_compile_options(quack PRIVATE -Wall -Wextra)
target_compile_features(quack PRIVATE cxx_std_17)
# # Add test cpp file
add_executable( quack-tests t/test_main.cpp ${files})
# # # Link test executable against gtest & gtest_main
target_compile_features(quack-tests PRIVATE cxx_std_11)
target_link_libraries(quack-tests gtest gtest_main)
gtest_discover_tests(quack-tests)
include_directories("src/" "vendor/luajit-2.0/dynasm")
