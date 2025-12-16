# LIMP Examples

# Macro to add an example
macro(add_limp_example category name sources)
    add_executable(${name} ${sources})
    target_link_libraries(${name} PRIVATE limp)
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/${category}"
    )
endmacro()

# 01 - Basic Examples (no ZMQ transport)
add_limp_example("01_basic" "simple_request" "01_basic/simple_request.cpp")
add_limp_example("01_basic" "simple_response" "01_basic/simple_response.cpp")

# 02 - Request-Reply Pattern (REQ-REP)
add_limp_example("02_request_reply" "client" "02_request_reply/client.cpp")
add_limp_example("02_request_reply" "server" "02_request_reply/server.cpp")

# 03 - Publish-Subscribe Pattern (PUB-SUB)
add_limp_example("03_pubsub" "publisher_subscriber" "03_pubsub/publisher_subscriber.cpp")
add_limp_example("03_pubsub" "subscriber_only" "03_pubsub/subscriber_only.cpp")

# 04 - Router-Dealer Patterns
add_limp_example("04_router_dealer" "router" "04_router_dealer/router.cpp")
add_limp_example("04_router_dealer" "dealer" "04_router_dealer/dealer.cpp")

# 05 - Message Broker Patterns
add_limp_example("05_broker" "proxy_broker" "05_broker/proxy_broker.cpp")
add_limp_example("05_broker" "router_broker" "05_broker/router_broker.cpp")
add_limp_example("05_broker" "broker_node" "05_broker/broker_node.cpp")
