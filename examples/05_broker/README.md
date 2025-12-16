# Message Brokers

Central intermediary for N:N node communication.

## Examples

### router_broker.cpp ⭐ **RECOMMENDED**
Custom routing logic with message inspection and destination-based routing.

**Run:**

**Linux/Mac:**
```bash
./build/examples/05_broker/router_broker
```

**Windows:**
```powershell
.\build\examples\05_broker\Release\router_broker.exe
```

**Use this for:** Production message brokering, HMI↔PLC communication, content-based routing

---

### broker_node.cpp
Example nodes communicating through router_broker.

**Run** (separate terminals):

**Linux/Mac:**
```bash
# Terminal 1: Start router broker
./build/examples/05_broker/router_broker

# Terminal 2: PLC node (connects to router broker)
NODE_TYPE=PLC ./build/examples/05_broker/broker_node

# Terminal 3: HMI node (connects to router broker)
NODE_TYPE=HMI ./build/examples/05_broker/broker_node

# Terminal 4: Logger node (connects to router broker)
NODE_TYPE=LOGGER ./build/examples/05_broker/broker_node
```

**Windows (PowerShell):**
```powershell
# Terminal 1: Start router broker
.\build\examples\05_broker\Release\router_broker.exe

# Terminal 2: PLC node (connects to router broker)
$env:NODE_TYPE="PLC"; .\build\examples\05_broker\Release\broker_node.exe

# Terminal 3: HMI node (connects to router broker)
$env:NODE_TYPE="HMI"; .\build\examples\05_broker\Release\broker_node.exe

# Terminal 4: Logger node (connects to router broker)
$env:NODE_TYPE="LOGGER"; .\build\examples\05_broker\Release\broker_node.exe
```

---

### direct_routing.cpp
Demonstrates explicit destination-based routing using `sendTo()` and `receive(source, dest, frame)`.

**Run:**

**Linux/Mac:**
```bash
./build/examples/05_broker/direct_routing
```

**Windows:**
```powershell
.\build\examples\05_broker\Release\direct_routing.exe
```

This example shows:
- Clients use `dealer.sendTo(destination, frame)` to specify message recipient
- Router uses `router.receive(sourceIdentity, destinationIdentity, frame)` to extract both source and destination
- Router forwards messages based on explicit destination identities
- Demonstrates the full sendTo/receive pattern for peer-to-peer routing

---

## Broker Types

- **router_broker** ⭐ (RECOMMENDED): Custom routing with destination-based message delivery
- **direct_routing**: Demonstrates sendTo/receive pattern with explicit destination addressing
