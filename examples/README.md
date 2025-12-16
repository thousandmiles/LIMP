# LIMP Examples

Examples demonstrating LIMP protocol with ZeroMQ transport.

## Build

```bash
./build.sh release-all
```

Executables: `build/examples/`

## Examples

### [01_basic/](01_basic/) - Protocol Basics
- `simple_request.cpp` - Build and parse LIMP messages (no network)
- `simple_response.cpp` - Response messages with different payload types

### [02_request_reply/](02_request_reply/) - REQ-REP Pattern
- `client.cpp` - Request client
- `server.cpp` - Reply server

### [03_pubsub/](03_pubsub/) - PUB-SUB Pattern
- `publisher_subscriber.cpp` - Publisher + subscriber in one program
- `subscriber_only.cpp` - Standalone subscriber

### [04_router_dealer/](04_router_dealer/) - ROUTER-DEALER Pattern
- `router.cpp` - ROUTER socket for routing
- `dealer.cpp` - DEALER socket for async requests

### [05_broker/](05_broker/) - Message Brokers
- `proxy_broker.cpp` - Automatic message forwarding
- `router_broker.cpp` - Custom routing logic
- `broker_node.cpp` - Example nodes using broker

## Usage

```bash
# Basic (no network)
./build/examples/01_basic/simple_request
./build/examples/01_basic/simple_response

# Request-Reply (separate terminals)
./build/examples/02_request_reply/server
./build/examples/02_request_reply/client

# Pub-Sub
./build/examples/03_pubsub/publisher_subscriber

# Router-Dealer (separate terminals)
./build/examples/04_router_dealer/router
./build/examples/04_router_dealer/dealer

# Broker (separate terminals)
./build/examples/05_broker/proxy_broker
NODE_TYPE=PLC ./build/examples/05_broker/broker_node
NODE_TYPE=HMI ./build/examples/05_broker/broker_node
```

See category READMEs for details.
