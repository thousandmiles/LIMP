# Message Brokers

Central intermediary for N:N node communication.

## Examples

### proxy_broker.cpp
Automatic message forwarding using `zmq::proxy()` (ROUTER-ROUTER).

**Run**:
```bash
./build/examples/05_broker/proxy_broker
```

- Frontend: `tcp://0.0.0.0:5555`
- Backend: `tcp://0.0.0.0:5556`

### router_broker.cpp
Custom routing logic with message inspection.

**Run**:
```bash
./build/examples/05_broker/router_broker
```

### broker_node.cpp
Example nodes communicating through broker.

**Run** (separate terminals):
```bash
# Terminal 1: Start broker
./build/examples/05_broker/proxy_broker

# Terminal 2: PLC node
NODE_TYPE=PLC ./build/examples/05_broker/broker_node

# Terminal 3: HMI node
NODE_TYPE=HMI ./build/examples/05_broker/broker_node

# Terminal 4: Logger node
NODE_TYPE=LOGGER ./build/examples/05_broker/broker_node
```

## Broker Types

- **proxy_broker**: Zero-copy forwarding, highest performance
- **router_broker**: Custom routing, message inspection, filtering
