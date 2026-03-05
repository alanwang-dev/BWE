# Simulation utilities: synthetic network data for validation and unit tests.
# Not compiled into the production library.
add_library(bwe_sim
  network_simulator.cpp
)
target_include_directories(bwe_sim
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(bwe_sim PUBLIC bbr_bwe)
