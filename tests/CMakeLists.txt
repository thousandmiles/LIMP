# Add simple test executable
add_executable(test_frame test_frame.cpp)
target_link_libraries(test_frame PRIVATE limp)

add_test(NAME FrameTest COMMAND test_frame)
