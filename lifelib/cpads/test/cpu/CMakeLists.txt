
file(GLOB_RECURSE cpu_unit_test_sources CONFIGURE_DEPENDS "unit/test_*.cpp")
add_executable(cpu_unit_test ${cpu_unit_test_sources} )
target_link_libraries(cpu_unit_test gtest_main gtest)

add_executable(fibmix fibmix.cpp)
add_executable(powermod powermod.cpp)
