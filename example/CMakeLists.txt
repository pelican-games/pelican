add_executable(example_minimum
    minimal/main.cpp
)
target_link_libraries(example_minimum PRIVATE pelican)
set_target_properties(example_minimum PROPERTIES CXX_STANDARD 20)

add_executable(example_simple_render
    simple_render/main.cpp
)
target_link_libraries(example_simple_render PRIVATE pelican)
set_target_properties(example_simple_render PROPERTIES CXX_STANDARD 20)

add_executable(example_instancing
    instancing/main.cpp
)
target_link_libraries(example_instancing PRIVATE pelican)
set_target_properties(example_instancing PROPERTIES CXX_STANDARD 20)

add_executable(example_key_input
    key_input/main.cpp
)
target_link_libraries(example_key_input PRIVATE pelican)
set_target_properties(example_key_input PROPERTIES CXX_STANDARD 20)
