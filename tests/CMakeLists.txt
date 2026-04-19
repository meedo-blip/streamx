# Create separate test executables
add_executable(test_frame test_frame.cpp)
add_executable(test_buffer test_buffer.cpp)
add_executable(test_encoder test_encoder.cpp)
add_executable(test_platforms test_platforms.cpp)

# Link all tests to the core library
target_link_libraries(test_frame PRIVATE streamx_core)
target_link_libraries(test_buffer PRIVATE streamx_core)
target_link_libraries(test_encoder PRIVATE streamx_core)
target_link_libraries(test_platforms PRIVATE streamx_core)

# Add tests to CTest
add_test(NAME FrameTest COMMAND test_frame)
add_test(NAME BufferTest COMMAND test_buffer)
add_test(NAME EncoderTest COMMAND test_encoder)
add_test(NAME PlatformsTest COMMAND test_platforms)
