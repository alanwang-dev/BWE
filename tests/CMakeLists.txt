# Each test file has its own main(); build one executable per test.
add_executable(test_signal_preprocessor test_signal_preprocessor.cpp)
target_link_libraries(test_signal_preprocessor PRIVATE bbr_bwe)
add_test(NAME test_signal_preprocessor COMMAND test_signal_preprocessor)

add_executable(test_bwe_core test_bwe_core.cpp)
target_link_libraries(test_bwe_core PRIVATE bwe_sim)
add_test(NAME test_bwe_core COMMAND test_bwe_core)

add_executable(test_quality_scorer test_quality_scorer.cpp)
target_link_libraries(test_quality_scorer PRIVATE bbr_bwe)
add_test(NAME test_quality_scorer COMMAND test_quality_scorer)

add_executable(test_network_simulator test_network_simulator.cpp)
target_link_libraries(test_network_simulator PRIVATE bwe_sim)
add_test(NAME test_network_simulator COMMAND test_network_simulator)

