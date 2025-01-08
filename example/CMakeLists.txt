add_executable(example_minimum
    minimal/main.cpp
)
target_link_libraries(example_minimum PRIVATE pelican)
set_target_properties(example_minimum PROPERTIES CXX_STANDARD 20)
