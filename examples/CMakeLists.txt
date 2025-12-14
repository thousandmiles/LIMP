add_executable(simple_request simple_request.cpp)
target_link_libraries(simple_request PRIVATE limp)

add_executable(simple_response simple_response.cpp)
target_link_libraries(simple_response PRIVATE limp)

add_executable(subscribe_example subscribe_example.cpp)
target_link_libraries(subscribe_example PRIVATE limp)

# ZeroMQ examples (only if ZMQ support is enabled)
if(LIMP_BUILD_ZMQ)
    add_executable(zmq_client_example zmq_client_example.cpp)
    target_link_libraries(zmq_client_example PRIVATE limp)

    add_executable(zmq_server_example zmq_server_example.cpp)
    target_link_libraries(zmq_server_example PRIVATE limp)

    add_executable(zmq_pubsub_example zmq_pubsub_example.cpp)
    target_link_libraries(zmq_pubsub_example PRIVATE limp)
endif()
