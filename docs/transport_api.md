# LIMP Transport API Reference

## Overview

The LIMP transport layer provides a consistent API for sending and receiving LIMP frames across different transport mechanisms. All transport implementations follow a unified pattern with high-level Frame-based methods and low-level Raw methods.

## Design Principles

### Method Hierarchy
1. **Frame-based methods** (high-level) - Handle serialization/deserialization
2. **Raw methods** (low-level) - Handle actual socket I/O
3. Frame methods call Raw methods internally

### API Patterns
- **Request/Reply**: Client/Server with strict send-receive alternation
- **Publish/Subscribe**: One-way message distribution with topic filtering
- **Router/Dealer**: Asynchronous N:N communication with identity-based routing

---

## Base Transport Class

All transport implementations inherit from `Transport` base class.

### Core Methods

```cpp
virtual TransportError send(const Frame &frame) = 0;
virtual TransportError receive(Frame &frame, int timeoutMs = -1) = 0;
```

### Optional Methods

```cpp
virtual TransportError sendRaw(const uint8_t *data, size_t size);
virtual std::ptrdiff_t receiveRaw(uint8_t *buffer, size_t maxSize);
```

**Note**: Raw methods have default implementations that return errors. Child classes override as needed.

---

## ZMQClient (REQ Socket)

**Pattern**: Synchronous request-reply client  
**Socket Type**: ZMQ_REQ  
**Semantics**: Must alternate send → receive → send → receive

### Public API

#### Connection
```cpp
TransportError connect(const std::string &endpoint);
```
Connect to server endpoint (e.g., `"tcp://127.0.0.1:5555"`).

#### High-Level Methods
```cpp
TransportError send(const Frame &frame) override;
TransportError receive(Frame &frame, int timeoutMs = -1) override;
```

#### Low-Level Methods
```cpp
TransportError sendRaw(const uint8_t *data, size_t size) override;
std::ptrdiff_t receiveRaw(uint8_t *buffer, size_t maxSize) override;
```

### Usage Example

```cpp
ZMQClient client;
client.connect("tcp://127.0.0.1:5555");

// Send request
auto request = MessageBuilder::request(0x10, 0x3000, 7, 1)
    .setPayload(42)
    .build();
client.send(request);

// Receive response
Frame response;
client.receive(response);
```

---

## ZMQServer (REP Socket)

**Pattern**: Synchronous request-reply server  
**Socket Type**: ZMQ_REP  
**Semantics**: Must alternate receive → send → receive → send

### Public API

#### Binding
```cpp
TransportError bind(const std::string &endpoint);
```
Bind to endpoint to accept connections (e.g., `"tcp://0.0.0.0:5555"`).

#### High-Level Methods
```cpp
TransportError send(const Frame &frame) override;
TransportError receive(Frame &frame, int timeoutMs = -1) override;
```

#### Low-Level Methods
```cpp
TransportError sendRaw(const uint8_t *data, size_t size) override;
std::ptrdiff_t receiveRaw(uint8_t *buffer, size_t maxSize) override;
```

### Usage Example

```cpp
ZMQServer server;
server.bind("tcp://0.0.0.0:5555");

// Receive request
Frame request;
server.receive(request);

// Send response
auto response = MessageBuilder::response(0x30, 0x3000, 1, 1)
    .setPayload(result)
    .build();
server.send(response);
```

---

## ZMQPublisher (PUB Socket)

**Pattern**: One-way message distribution  
**Socket Type**: ZMQ_PUB  
**Semantics**: Send-only, no receive

### Public API

#### Binding
```cpp
TransportError bind(const std::string &endpoint);
```

#### Publish Methods (Primary API)
```cpp
TransportError publish(const std::string &topic, const Frame &frame);
TransportError publishRaw(const std::string &topic, const uint8_t *data, size_t size);
```

**Note**: Topic parameter is required. Use `""` for broadcast to all subscribers.

#### Base Class Overrides (Private - Return Errors)
```cpp
TransportError send(const Frame &frame) override;           // Returns InternalError
TransportError sendRaw(const uint8_t *data, size_t size) override;  // Returns InternalError
TransportError receive(Frame &frame, int timeoutMs = -1) override;  // Returns InternalError
std::ptrdiff_t receiveRaw(uint8_t *buffer, size_t maxSize) override;  // Returns -1
```

### Usage Example

```cpp
ZMQPublisher publisher;
publisher.bind("tcp://0.0.0.0:5556");

// Publish to specific topic
auto event = MessageBuilder::event(0x40, 0x5000, 1, 3)
    .setPayload(sensorData)
    .build();
publisher.publish("sensor/temperature", event);

// Broadcast to all (empty topic)
publisher.publish("", broadcastEvent);
```

---

## ZMQSubscriber (SUB Socket)

**Pattern**: Receive messages with topic filtering  
**Socket Type**: ZMQ_SUB  
**Semantics**: Receive-only, no send

### Public API

#### Connection
```cpp
TransportError connect(const std::string &endpoint);
```

#### Subscription Management
```cpp
TransportError subscribe(const std::string &topic = "");
TransportError unsubscribe(const std::string &topic);
```

**Note**: Must call `subscribe()` before receiving. Use `subscribe("")` to receive all messages.

#### High-Level Methods
```cpp
TransportError receive(Frame &frame, int timeoutMs = -1) override;
std::ptrdiff_t receiveRaw(uint8_t *buffer, size_t maxSize) override;
```

**Note**: Topic prefix is automatically stripped from received messages.

#### Base Class Overrides (Private - Return Errors)
```cpp
TransportError send(const Frame &frame) override;        // Returns InternalError
TransportError sendRaw(const uint8_t *data, size_t size) override; // Returns InternalError
```

### Usage Example

```cpp
ZMQSubscriber subscriber;
subscriber.connect("tcp://127.0.0.1:5556");
subscriber.subscribe("sensor/");  // Receive all sensor topics

Frame frame;
while (true) {
    if (subscriber.receive(frame) == TransportError::None) {
        // Process frame (topic already stripped)
    }
}
```

---

## ZMQDealer (DEALER Socket)

**Pattern**: Asynchronous client with optional routing  
**Socket Type**: ZMQ_DEALER  
**Semantics**: No send-receive order enforcement

### Public API

#### Connection & Identity
```cpp
TransportError setIdentity(const std::string &identity);  // Must call before connect
TransportError connect(const std::string &endpoint);
const std::string &getIdentity() const;
```

#### High-Level Methods (Without Routing)
```cpp
TransportError send(const Frame &frame) override;
TransportError receive(Frame &frame, int timeoutMs = -1) override;
```

**Message Format**: `[delimiter][data]` (2 parts)  
**Router Receives**: `[dealer_identity][delimiter][data]` (3 parts)

#### High-Level Methods (With Routing)
```cpp
TransportError send(const std::string &destinationIdentity, const Frame &frame);
TransportError receive(std::string &sourceIdentity, Frame &frame, int timeoutMs = -1);
```

**Message Format**: `[dest_identity][delimiter][data]` (3 parts)  
**Router Receives**: `[dealer_identity][dest_identity][delimiter][data]` (4 parts)

#### Low-Level Methods
```cpp
TransportError sendRaw(const uint8_t *data, size_t size) override;
TransportError sendRaw(const std::string &destinationIdentity, const uint8_t *data, size_t size);

std::ptrdiff_t receiveRaw(uint8_t *buffer, size_t maxSize) override;
std::ptrdiff_t receiveRaw(std::string &sourceIdentity, uint8_t *buffer, size_t maxSize);
```

### Usage Example

```cpp
ZMQDealer dealer;
dealer.setIdentity("worker-001");
dealer.connect("tcp://127.0.0.1:5555");

// Send without routing (direct to router)
auto request = MessageBuilder::request(0x10, 0x3000, 7, 1).build();
dealer.send(request);

// Send with routing (router forwards to specific destination)
dealer.send("target-node", request);

// Receive with source identity
std::string sourceId;
Frame response;
dealer.receive(sourceId, response);
```

---

## ZMQRouter (ROUTER Socket)

**Pattern**: Asynchronous server with identity-based routing  
**Socket Type**: ZMQ_ROUTER  
**Semantics**: Routes messages by client identity

### Public API

#### Binding
```cpp
TransportError bind(const std::string &endpoint);
```

#### High-Level Methods (Source Identity Only)
```cpp
TransportError receive(std::string &sourceIdentity, Frame &frame, int timeoutMs = -1);
TransportError send(const std::string &clientIdentity, const Frame &frame);
```

**Dealer Sends**: `[delimiter][data]` (2 parts)  
**Router Receives**: `[dealer_identity][delimiter][data]` (3 parts, identity auto-added by ZMQ)

#### High-Level Methods (Source + Destination)
```cpp
TransportError receive(std::string &sourceIdentity, std::string &destinationIdentity, 
                       Frame &frame, int timeoutMs = -1);
TransportError send(const std::string &clientIdentity, const std::string &sourceIdentity, 
                    const Frame &frame);
```

**Dealer Sends**: `[dest_identity][delimiter][data]` (3 parts)  
**Router Receives**: `[dealer_identity][dest_identity][delimiter][data]` (4 parts)

#### Low-Level Methods
```cpp
TransportError sendRaw(const std::vector<uint8_t> &clientIdentity, 
                       const uint8_t *data, size_t size);
TransportError sendRaw(const std::vector<uint8_t> &clientIdentity,
                       const std::vector<uint8_t> &sourceIdentity,
                       const uint8_t *data, size_t size);

std::ptrdiff_t receiveRaw(std::vector<uint8_t> &identity, 
                          uint8_t *buffer, size_t maxSize);
std::ptrdiff_t receiveRaw(std::vector<uint8_t> &sourceIdentity,
                          std::vector<uint8_t> &destinationIdentity,
                          uint8_t *buffer, size_t maxSize);
```

#### Base Class Overrides (Private - Return Errors)
```cpp
TransportError send(const Frame &frame) override;        // Returns InternalError
TransportError sendRaw(const uint8_t *data, size_t size) override;  // Returns InternalError
TransportError receive(Frame &frame, int timeoutMs = -1) override;  // Returns InternalError
std::ptrdiff_t receiveRaw(uint8_t *buffer, size_t maxSize) override;  // Returns -1
```

**Note**: Router requires identity-based methods. Base class methods return errors.

### Usage Example

```cpp
ZMQRouter router;
router.bind("tcp://0.0.0.0:5555");

// Receive from dealer (no routing)
std::string dealerId;
Frame request;
router.receive(dealerId, request);

// Send back to same dealer
auto response = MessageBuilder::response(0x30, 0x3000, 1, 1).build();
router.send(dealerId, response);

// Receive with routing info
std::string sourceId, destId;
router.receive(sourceId, destId, request);

// Forward to destination with source info
router.send(destId, sourceId, response);
```

---

## Error Handling

### TransportError Enum

```cpp
enum class TransportError {
    None = 0,              // Success
    ConnectionFailed,      // Failed to establish connection
    BindFailed,            // Failed to bind to endpoint
    SendFailed,            // Failed to send data
    ReceiveFailed,         // Failed to receive data
    Timeout,               // Operation timed out
    InvalidEndpoint,       // Invalid endpoint format
    SocketClosed,          // Socket is closed
    NotConnected,          // Not connected to endpoint
    SerializationFailed,   // Failed to serialize frame
    DeserializationFailed, // Failed to deserialize frame
    InvalidFrame,          // Frame validation failed
    AlreadyConnected,      // Already connected/bound
    ConfigurationError,    // Invalid configuration
    InternalError          // Unspecified internal error
};
```

### Error to String
```cpp
const char *toString(TransportError error) noexcept;
```

---

## API Consistency Rules

### 1. Method Naming
- **Frame-based**: `send(Frame)`, `receive(Frame)`
- **Raw methods**: `sendRaw()`, `receiveRaw()`
- **Pub/Sub specific**: `publish()`, `publishRaw()`, `subscribe()`

### 2. Call Hierarchy
- Frame methods MUST call Raw methods internally
- Raw methods handle actual socket I/O
- Frame methods handle serialization/deserialization

### 3. Return Types
- **TransportError**: For Frame-based send/receive operations
- **std::ptrdiff_t**: For Raw receive operations (bytes received, 0=timeout, -1=error)

### 4. Override Consistency
- All base class overrides marked with `override` keyword
- Unsupported methods (e.g., `send()` on subscriber) return `InternalError`

### 5. Identity Types
- **Frame methods**: Use `std::string` for identity
- **Raw methods**: Use `std::vector<uint8_t>` for identity (binary safe)
- Conversion handled internally

### 6. Topic Requirement (Pub/Sub)
- All publish methods require explicit `topic` parameter
- Use empty string `""` for broadcast
- Subscribe requires `subscribe()` call before receiving

---

## Best Practices

### 1. Error Checking
```cpp
if (client.send(frame) != TransportError::None) {
    // Handle error
}
```

### 2. Timeout Handling
```cpp
auto error = server.receive(frame, 1000);  // 1 second timeout
if (error == TransportError::Timeout) {
    // Handle timeout
}
```

### 3. Identity Management
```cpp
// Set identity before connect
dealer.setIdentity("worker-001");
dealer.connect(endpoint);
```

### 4. Topic Filtering
```cpp
// Subscribe to multiple topics
subscriber.subscribe("sensor/");
subscriber.subscribe("alarm/");
subscriber.subscribe("");  // Or receive all
```

### 5. Resource Cleanup
```cpp
// RAII - no manual cleanup needed
{
    ZMQClient client;
    client.connect(endpoint);
    // ... use client ...
}  // Automatically cleaned up
```

---

## Pattern Comparison

| Feature | Client/Server | Pub/Sub | Router/Dealer |
|---------|--------------|---------|---------------|
| **Communication** | Synchronous | One-way | Asynchronous |
| **Order** | Strict alternation | N/A | No enforcement |
| **Routing** | Direct | Topic-based | Identity-based |
| **N:N** | No | Yes (1:N) | Yes |
| **Use Case** | Request/Reply | Events, Logs | Distributed systems |
| **Reliability** | High | Best effort | Configurable |

---

## Thread Safety

**Note**: ZeroMQ sockets are **NOT thread-safe**. Each transport instance should be used from a single thread. For multi-threaded use:
- Create separate transport instances per thread
- Use message queues for inter-thread communication
- Share context (managed internally by ZMQTransport base class)

---

## Performance Considerations

### 1. Buffer Sizing
Default buffer size for Raw receive: 4096 bytes. Adjust if needed for large messages.

### 2. Move Semantics
All transport classes support move operations:
```cpp
ZMQClient client1 = std::move(client2);  // Efficient ownership transfer
```

### 3. Zero-Copy
Frame-based methods use move semantics internally to avoid unnecessary copies.

### 4. Topic Filtering
Subscriber topic filtering happens at ZeroMQ level (efficient) before reaching application.

---

## Version

**LIMP Protocol Version**: As defined in `limp.hpp`  
**Last Updated**: December 22, 2025
