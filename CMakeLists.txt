cmake_minimum_required(VERSION 3.0)
project(quack)

set(CMAKE_CXX_STANDARD 11)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
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
  )

unset(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
add_executable(quack-exe ${main})
add_library(quack ${files})

set_target_properties(quack-exe PROPERTIES OUTPUT_NAME quack)
set_target_properties(quack PROPERTIES C_STANDARD 99)
set_target_properties(quack-exe PROPERTIES C_STANDARD 99)
target_link_libraries(quack gccjit)
target_link_libraries(quack-exe readline quack)
target_compile_options(quack PRIVATE -Wall -Wextra)
target_compile_options(quack-exe PRIVATE -Wall -Wextra)
target_compile_features(quack PRIVATE cxx_std_17)
add_executable( quack-tests t/test_main.cpp)
target_link_libraries(quack-tests gtest gtest_main gccjit quack)
gtest_discover_tests(quack-tests)
include_directories("src/" "/usr/lib/gcc/x86_64-linux-gnu/12/include/ ")
