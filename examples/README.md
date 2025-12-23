# LIMP Examples

Examples demonstrating LIMP protocol with ZeroMQ transport.

## Build

**Linux/Mac:**

```bash
./build.sh release-all
```

**Windows:**

```powershell
.\build.ps1 release-all
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

- `router_broker.cpp` - Custom routing logic with content-based routing ⭐ **RECOMMENDED**
- `broker_node.cpp` - Example nodes (HMI, PLC, Logger) using router broker
- `direct_routing.cpp` - Demonstrates send/receive pattern with explicit destination addressing

## Usage

**Linux/Mac:**

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

# Message Broker (separate terminals)
./build/examples/05_broker/router_broker
NODE_TYPE=PLC ./build/examples/05_broker/broker_node
NODE_TYPE=HMI ./build/examples/05_broker/broker_node
NODE_TYPE=LOGGER ./build/examples/05_broker/broker_node

# Direct Routing (runs in single process with multiple threads)
./build/examples/05_broker/direct_routing
```

**Windows (PowerShell):**

```powershell
# Basic (no network)
.\build\examples\01_basic\Release\simple_request.exe
.\build\examples\01_basic\Release\simple_response.exe

# Request-Reply (separate terminals)
.\build\examples\02_request_reply\Release\server.exe
.\build\examples\02_request_reply\Release\client.exe

# Pub-Sub
.\build\examples\03_pubsub\Release\publisher_subscriber.exe

# Router-Dealer (separate terminals)
.\build\examples\04_router_dealer\Release\router.exe
.\build\examples\04_router_dealer\Release\dealer.exe

# Message Broker (separate terminals)
.\build\examples\05_broker\Release\router_broker.exe
$env:NODE_TYPE="PLC"; .\build\examples\05_broker\Release\broker_node.exe
$env:NODE_TYPE="HMI"; .\build\examples\05_broker\Release\broker_node.exe
$env:NODE_TYPE="LOGGER"; .\build\examples\05_broker\Release\broker_node.exe

# Direct Routing (runs in single process with multiple threads)
.\build\examples\05_broker\Release\direct_routing.exe
```

See category READMEs for details.
