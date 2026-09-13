find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME pgorca_learnable_test
    COMMAND ${Python3_EXECUTABLE} -B -m unittest discover -s tools/learnable/tests -t .)
set_tests_properties(pgorca_learnable_test PROPERTIES
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    LABELS "learnable")
