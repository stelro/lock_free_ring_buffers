cmake_minimum_required(VERSION 3.18)

# ---- Project ----
project(ring_buffers VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # Generate compile_commands.json for static analysis

# ---- Build Configuration ----
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ---- Source Files ----
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cxx" "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.h" "src/*.hpp" "src/*.hxx")

# ---- Main Executable ----
add_executable(${PROJECT_NAME} src/main.cpp)

# ---- Library (if you have multiple source files) ----
# Uncomment and modify as needed:
# add_library(${PROJECT_NAME}_lib ${SOURCES})
# target_include_directories(${PROJECT_NAME}_lib PUBLIC src)
# target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_lib)

# ---- Tests ----
include(CTest)
enable_testing()

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.0.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Individual test executables - each .cpp file becomes its own executable
file(GLOB TEST_SOURCES "test/*.cpp" "test/*.cxx" "test/*.cc")
foreach(TEST_FILE ${TEST_SOURCES})
  get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
  add_executable(${TEST_NAME} ${TEST_FILE})
  target_link_libraries(${TEST_NAME} gtest_main)
  target_include_directories(${TEST_NAME} PRIVATE src)
  
  # Optional: link with project library if you have shared code
  # target_link_libraries(${TEST_NAME} ${PROJECT_NAME}_lib)
  
  # Register each test executable with CTest
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# ---- Benchmarks ----
# Fetch Google Benchmark
FetchContent_Declare(
  googlebenchmark
  URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

# Individual benchmark executables - each .cpp file becomes its own executable
file(GLOB BENCH_SOURCES "bench/*.cpp" "bench/*.cxx" "bench/*.cc")
foreach(BENCH_FILE ${BENCH_SOURCES})
  get_filename_component(BENCH_NAME ${BENCH_FILE} NAME_WE)
  add_executable(${BENCH_NAME} ${BENCH_FILE})
  target_link_libraries(${BENCH_NAME} benchmark::benchmark)
  target_include_directories(${BENCH_NAME} PRIVATE src)
  
  # Optional: link with project library if you have shared code
  # target_link_libraries(${BENCH_NAME} ${PROJECT_NAME}_lib)
endforeach()

# ---- Custom Targets ----
# Example: add more executables here
# add_executable(${PROJECT_NAME}_tool tools/tool.cpp)
# target_link_libraries(${PROJECT_NAME}_tool ${PROJECT_NAME}_lib)

# ---- Installation ----
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# ---- Packaging ----
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)
