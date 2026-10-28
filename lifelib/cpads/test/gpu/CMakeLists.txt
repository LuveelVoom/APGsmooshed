
file(GLOB_RECURSE gpu_unit_test_sources CONFIGURE_DEPENDS "unit/test_*.cu")
add_executable(gpu_unit_test ${gpu_unit_test_sources} )
target_link_libraries(gpu_unit_test gtest_main gtest)

add_executable(gpu_gaussian_cdf "test_gaussian_cdf.cu")
add_executable(gpu_gaussian_tail "test_gaussian_tail.cu")
