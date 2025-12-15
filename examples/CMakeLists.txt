add_executable(simple_request simple_request.cpp)
target_link_libraries(simple_request PRIVATE limp)

add_executable(simple_response simple_response.cpp)
target_link_libraries(simple_response PRIVATE limp)

add_executable(subscribe_example subscribe_example.cpp)
target_link_libraries(subscribe_example PRIVATE limp)

# ZeroMQ examples (only if ZMQ support is enabled)
if(LIMP_BUILD_ZMQ)
    add_executable(zmq_client_example zmq_client_example.cpp)
    target_link_libraries(zmq_client_example PRIVATE limp cppzmq)

    add_executable(zmq_server_example zmq_server_example.cpp)
    target_link_libraries(zmq_server_example PRIVATE limp cppzmq)

    add_executable(zmq_pubsub_example zmq_pubsub_example.cpp)
    target_link_libraries(zmq_pubsub_example PRIVATE limp cppzmq)

    add_executable(zmq_router_example zmq_router_example.cpp)
    target_link_libraries(zmq_router_example PRIVATE limp cppzmq)

    add_executable(zmq_dealer_example zmq_dealer_example.cpp)
    target_link_libraries(zmq_dealer_example PRIVATE limp cppzmq)

    add_executable(zmq_proxy_example zmq_proxy_example.cpp)
    target_link_libraries(zmq_proxy_example PRIVATE limp cppzmq)

    add_executable(zmq_broker_node_example zmq_broker_node_example.cpp)
    target_link_libraries(zmq_broker_node_example PRIVATE limp cppzmq)

    add_executable(central_server_router central_server_router.cpp)
    target_link_libraries(central_server_router PRIVATE limp cppzmq)

    add_executable(central_server_proxy central_server_proxy.cpp)
    target_link_libraries(central_server_proxy PRIVATE limp cppzmq)
endif()
