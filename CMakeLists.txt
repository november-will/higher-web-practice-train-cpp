cmake_minimum_required(VERSION 3.20)

project(SortingHill LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
  add_compile_options(/W4 /permissive- /utf-8)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(train_core
  train/sorting_hill.cpp
  train/sorting_operator.cpp
  train/sorting_reporter.cpp
)

target_include_directories(train_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/train
)

add_executable(train_app
  train/main.cpp
)

target_link_libraries(train_app PRIVATE train_core)

include(CTest)

if (BUILD_TESTING)
  include(FetchContent)

  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )

  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(googletest)

  add_executable(train_tests
    tests/station_runtime_gtest.cpp
  )

  target_include_directories(train_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/train
  )

  target_link_libraries(train_tests PRIVATE
    train_core
    GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(train_tests)
endif()
