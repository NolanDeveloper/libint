add_executable(libint_unit_test main.c)
target_link_libraries(libint_unit_test PUBLIC libint)
add_test(libint_unit_test libint_unit_test)
