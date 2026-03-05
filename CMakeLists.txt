cmake_minimum_required(VERSION 3.15)

project(bbr_bwe_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BBR_BWE_BUILD_TESTS "Build unit tests" ON)

add_library(bbr_bwe
  src/network/network_stats.h
  src/bwe/signal_preprocessor.h
  src/bwe/signal_preprocessor.cpp
  src/bwe/bwe_core.h
  src/bwe/bwe_core_impl.cpp
  src/bwe/quality_scorer.h
  src/bwe/quality_scorer.cpp
  src/api/bwe_api.h
  src/api/bwe_api.cpp
)

target_include_directories(bbr_bwe
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(sim)
add_subdirectory(tools)

if(BBR_BWE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

