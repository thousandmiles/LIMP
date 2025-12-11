add_executable(simple_request simple_request.cpp)
target_link_libraries(simple_request PRIVATE limp)

add_executable(simple_response simple_response.cpp)
target_link_libraries(simple_response PRIVATE limp)

add_executable(subscribe_example subscribe_example.cpp)
target_link_libraries(subscribe_example PRIVATE limp)
