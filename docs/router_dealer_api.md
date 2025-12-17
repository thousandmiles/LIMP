# ROUTER-DEALER API Summary

## Method Matching Overview

The DEALER and ROUTER send/receive methods must be used in matching pairs. The message format transformations are handled automatically by ZeroMQ.

---

## Pattern 1: Simple Communication (No Source Identity)

### DEALER → ROUTER
```cpp
// DEALER sends
dealer.send(frame);
// Sends: [delimiter][data] (2 parts)

// ROUTER receives  
router.receive(sourceIdentity, frame);
// Receives: [dealer_identity][delimiter][data] (3 parts)
// ZeroMQ automatically adds dealer_identity
```

### ROUTER → DEALER
```cpp
// ROUTER sends
router.send(clientIdentity, frame);
// Sends: [client_identity][delimiter][data] (3 parts)

// DEALER receives
dealer.receive(frame);
// Receives: [delimiter][data] (2 parts)
// ZeroMQ automatically strips client_identity (matches this dealer)
```

---

## Pattern 2: Routed Communication (With Source Identity)

### DEALER → ROUTER (with destination)
```cpp
// DEALER sends with destination
dealer.send(destinationIdentity, frame);
// Sends: [dest_identity][delimiter][data] (3 parts)

// ROUTER receives
router.receive(sourceIdentity, destinationIdentity, frame);
// Receives: [dealer_identity][dest_identity][delimiter][data] (4 parts)
// ZeroMQ automatically adds dealer_identity as first frame
```

### ROUTER → DEALER (with source)
```cpp
// ROUTER sends with source identity
router.send(clientIdentity, sourceIdentity, frame);
// Sends: [client_identity][source_identity][delimiter][data] (4 parts)

// DEALER receives
dealer.receive(sourceIdentity, frame);
// Receives: [source_identity][delimiter][data] (3 parts)
// ZeroMQ automatically strips client_identity (matches this dealer)
```

---

## Complete Method Reference

### ZMQDealer Methods

| Method | Message Format | Paired With | Use Case |
|--------|---------------|-------------|----------|
| `send(frame)` | `[delimiter][data]` | `router.receive(src, frame)` | Simple send to router |
| `send(dest, frame)` | `[dest][delimiter][data]` | `router.receive(src, dest, frame)` | Send with routing info |
| `receive(frame)` | Receives: `[delimiter][data]` | `router.send(client, frame)` | Receive without source |
| `receive(src, frame)` | Receives: `[src][delimiter][data]` | `router.send(client, src, frame)` | Receive with source |

### ZMQRouter Methods

| Method | Message Format | Paired With | Use Case |
|--------|---------------|-------------|----------|
| `receive(src, frame)` | Receives: `[src][delimiter][data]` | `dealer.send(frame)` | Receive simple message |
| `receive(src, dest, frame)` | Receives: `[src][dest][delimiter][data]` | `dealer.send(dest, frame)` | Receive routed message |
| `send(client, frame)` | `[client][delimiter][data]` | `dealer.receive(frame)` | Send without source |
| `send(client, src, frame)` | `[client][src][delimiter][data]` | `dealer.receive(src, frame)` | Send with source |

---

## Key Points

1. **ZeroMQ handles identity frames automatically:**
   - ROUTER adds dealer identity when receiving
   - DEALER strips its own identity when receiving

2. **Message parts always include an empty delimiter frame**
   - Required by ZeroMQ envelope protocol
   - Separates routing frames from data frames

3. **Matching is critical:**
   - `dealer.send(frame)` ↔ `router.receive(src, frame)`
   - `dealer.send(dest, frame)` ↔ `router.receive(src, dest, frame)`
   - `router.send(client, frame)` ↔ `dealer.receive(frame)`
   - `router.send(client, src, frame)` ↔ `dealer.receive(src, frame)`

4. **Common patterns:**
   - **Client-Server:** Use Pattern 1 (no source identity)
   - **Routing/Broker:** Use Pattern 2 (with source identity for forwarding)
   - **Peer-to-Peer through broker:** Dealers send with destination, broker forwards with source

---

## Example: Peer-to-Peer Routing

```cpp
// Client A sends to Client B through router
dealer_A.send("CLIENT_B", frame);  // [CLIENT_B][delimiter][data]

// Router receives
router.receive(srcId, destId, frame);  
// srcId = "CLIENT_A", destId = "CLIENT_B"

// Router forwards to Client B
router.send("CLIENT_B", "CLIENT_A", frame);  // [CLIENT_B][CLIENT_A][delimiter][data]

// Client B receives
dealer_B.receive(sourceId, frame);  
// sourceId = "CLIENT_A" (knows who sent it)
```
