# LIMP Transactional API Design

**Version**: 0.4  
**Date**: December 23, 2025  
**Status**: Design

---

## Overview

This document specifies a thread-safe transactional layer built on top of LIMP's transport layer. The design provides:

1. **Thread-safe wrappers** for ZMQClient, ZMQDealer, and ZMQRouter
2. **Consistent API naming** matching the transport layer (see `transport_api.md`)
3. **Synchronous and asynchronous APIs** with async suffix (e.g., `receiveAsync()`)
4. **Automatic transaction tracking** via TransactionTracker
5. **Pattern-specific implementations** respecting ZeroMQ semantics

---

## Design Principles

### 1. Thread Safety
- All wrapper classes are **thread-safe** using internal mutexes
- Multiple threads can safely call methods on the same instance
- TransactionTracker is thread-safe for concurrent transaction operations

### 2. API Consistency
- Wrapper methods match transport layer naming (from `transport_api.md`)
- Synchronous methods: same names as transport layer (`send()`, `receive()`)
- Asynchronous methods: append `Async` suffix (`sendAsync()`, `receiveAsync()`)

### 3. Transaction Management
- AttrID field (16-bit) used as transaction ID
- Automatic transaction registration on send
- Automatic transaction completion on receive
- Thread-safe pending transaction tracking

### 4. Pattern Fidelity
- Each wrapper respects underlying ZeroMQ pattern semantics
- No cross-pattern assumptions or templates
- Identity-based routing preserved for Router/Dealer

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Application Layer                          │
│  (Business Logic, Broker Patterns, State Machines)          │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              Transactional Layer (Thread-Safe)               │
│  ┌──────────────────┐  ┌──────────────────┐                │
│  │ TransactionalClient│  │ TransactionalDealer│              │
│  │  (REQ wrapper)    │  │  (DEALER wrapper)│                │
│  └──────────────────┘  └──────────────────┘                │
│  ┌──────────────────┐  ┌──────────────────┐                │
│  │ TransactionalRouter│  │ TransactionTracker│              │
│  │  (ROUTER wrapper) │  │  (Core tracker)  │                │
│  └──────────────────┘  └──────────────────┘                │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│               Transport Layer (NOT Thread-Safe)              │
│    ZMQClient    ZMQServer    ZMQDealer    ZMQRouter         │
│    ZMQPublisher    ZMQSubscriber    ZMQProxy                │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Components

### TransactionTracker

**Purpose**: Thread-safe transaction lifecycle management

**Thread Safety**: Fully thread-safe with internal mutex

**Responsibilities**:
- Register pending transactions
- Complete transactions (success/failure)
- Track transaction timeouts
- Query transaction status

#### Public Interface

```cpp
class TransactionTracker {
public:
    struct TransactionInfo {
        uint16_t transactionId;
        std::chrono::steady_clock::time_point timestamp;
        std::string sourceIdentity;      // For routing (optional)
        std::string destinationIdentity; // For routing (optional)
        bool completed;
    };

    // Transaction lifecycle (thread-safe)
    uint16_t registerTransaction(const std::string &sourceId = "",
                                  const std::string &destId = "");
    bool completeTransaction(uint16_t transactionId, bool success = true);
    
    // Query methods (thread-safe)
    bool isPending(uint16_t transactionId) const;
    std::optional<TransactionInfo> getTransaction(uint16_t transactionId) const;
    std::vector<uint16_t> getPendingTransactions() const;
    
    // Timeout management (thread-safe)
    std::vector<uint16_t> getTimedOutTransactions(
        std::chrono::milliseconds timeout) const;
    void cleanupTimedOutTransactions(std::chrono::milliseconds timeout);
    
    // Statistics (thread-safe)
    size_t getPendingCount() const;
    void clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<uint16_t, TransactionInfo> transactions_;
    uint16_t nextTransactionId_ = 1;
};
```

#### Implementation Notes

- Uses `std::mutex` for all operations
- Transaction IDs wrap around at 65535 → 1 (avoid 0 for "no transaction")
- Timestamp captured at registration for timeout detection
- Identity fields optional (empty for Client pattern)

---

## Wrapper Classes

### TransactionalClient

**Purpose**: Thread-safe wrapper for ZMQClient with transaction tracking

**Thread Safety**: Thread-safe (internal mutex)

**Pattern**: Synchronous request-reply client (REQ socket)

#### Public Interface

```cpp
class TransactionalClient {
public:
    explicit TransactionalClient(std::shared_ptr<TransactionTracker> tracker);
    
    // Connection (thread-safe)
    TransportError connect(const std::string &endpoint);
    
    // Synchronous Frame API (thread-safe)
    TransportError send(const Frame &frame);
    TransportError receive(Frame &frame, int timeoutMs = -1);
    
    // Synchronous Raw API (thread-safe)
    TransportError sendRaw(const uint8_t *data, size_t size);
    std::ptrdiff_t receiveRaw(uint8_t *buffer, size_t maxSize);
    
    // Asynchronous Frame API (thread-safe)
    std::future<TransportError> sendAsync(const Frame &frame);
    std::future<std::pair<TransportError, Frame>> receiveAsync(int timeoutMs = -1);
    
    // Asynchronous Raw API (thread-safe)
    std::future<TransportError> sendRawAsync(std::vector<uint8_t> data);
    std::future<std::pair<std::ptrdiff_t, std::vector<uint8_t>>> receiveRawAsync(
        size_t maxSize);
    
    // Transaction query (thread-safe)
    std::optional<uint16_t> getLastTransactionId() const;

private:
    ZMQClient client_;
    std::shared_ptr<TransactionTracker> tracker_;
    mutable std::mutex mutex_;
    std::optional<uint16_t> lastTransactionId_;
};
```

#### Behavior

**Synchronous Methods**:
- `send()`: Extracts AttrID from frame, registers transaction, forwards to client
- `receive()`: Receives from client, extracts AttrID, completes transaction

**Asynchronous Methods**:
- `sendAsync()`: Returns `std::future<TransportError>`, executes send in thread pool
- `receiveAsync()`: Returns `std::future<std::pair<TransportError, Frame>>`, executes receive in thread pool
- `sendRawAsync()`: Takes ownership of the payload by moving an `std::vector<uint8_t>` into the background task

**Thread Safety**:
- All methods lock `mutex_` before accessing `client_` or internal state
- Async operations capture necessary state and release lock during I/O

#### Usage Example

```cpp
auto tracker = std::make_shared<TransactionTracker>();
TransactionalClient client(tracker);
client.connect("tcp://127.0.0.1:5555");

// Synchronous usage
auto request = MessageBuilder::request(0x10, 0x3000, 7, 1)
    .setPayload(42)
    .build();
client.send(request);

Frame response;
client.receive(response);

// Asynchronous usage
auto sendFuture = client.sendAsync(request);
auto receiveFuture = client.receiveAsync(5000);

auto [error, response] = receiveFuture.get();
```

---

### TransactionalDealer

**Purpose**: Thread-safe wrapper for ZMQDealer with transaction tracking

**Thread Safety**: Thread-safe (internal mutex)

**Pattern**: Asynchronous client with identity-based routing (DEALER socket)

#### Public Interface

```cpp
class TransactionalDealer {
public:
    explicit TransactionalDealer(std::shared_ptr<TransactionTracker> tracker);
    
    // Connection & Identity (thread-safe)
    TransportError setIdentity(const std::string &identity);
    TransportError connect(const std::string &endpoint);
    const std::string &getIdentity() const;
    
    // Synchronous Frame API - Without Routing (thread-safe)
    TransportError send(const Frame &frame);
    TransportError receive(Frame &frame, int timeoutMs = -1);
    
    // Synchronous Frame API - With Routing (thread-safe)
    TransportError send(const std::string &destinationIdentity, const Frame &frame);
    TransportError receive(std::string &sourceIdentity, Frame &frame, 
                           int timeoutMs = -1);
    
    // Synchronous Raw API - Without Routing (thread-safe)
    TransportError sendRaw(const uint8_t *data, size_t size);
    std::ptrdiff_t receiveRaw(uint8_t *buffer, size_t maxSize);
    
    // Synchronous Raw API - With Routing (thread-safe)
    TransportError sendRaw(const std::string &destinationIdentity, 
                           const uint8_t *data, size_t size);
    std::ptrdiff_t receiveRaw(std::string &sourceIdentity, 
                              uint8_t *buffer, size_t maxSize);
    
    // Asynchronous Frame API - Without Routing (thread-safe)
    std::future<TransportError> sendAsync(const Frame &frame);
    std::future<std::pair<TransportError, Frame>> receiveAsync(int timeoutMs = -1);
    
    // Asynchronous Frame API - With Routing (thread-safe)
    std::future<TransportError> sendAsyncWithRouting(
        const std::string &destinationIdentity, const Frame &frame);
    std::future<std::tuple<TransportError, std::string, Frame>> receiveAsyncWithRouting(
        int timeoutMs = -1);
    
    // Asynchronous Raw API - Without Routing (thread-safe)
    std::future<TransportError> sendRawAsync(std::vector<uint8_t> data);
    std::future<std::pair<std::ptrdiff_t, std::vector<uint8_t>>> receiveRawAsync(
        size_t maxSize);
    
    // Asynchronous Raw API - With Routing (thread-safe)
    std::future<TransportError> sendRawAsyncWithRouting(
        const std::string &destinationIdentity, std::vector<uint8_t> data);
    std::future<std::tuple<std::ptrdiff_t, std::string, std::vector<uint8_t>>>
        receiveRawAsyncWithRouting(size_t maxSize);
    
    // Transaction query (thread-safe)
    std::optional<uint16_t> getLastTransactionId() const;

private:
    ZMQDealer dealer_;
    std::shared_ptr<TransactionTracker> tracker_;
    mutable std::mutex mutex_;
    std::optional<uint16_t> lastTransactionId_;
};
```

#### Behavior

**Synchronous Methods**:
- `send()`: Extracts AttrID, registers transaction with destination identity, forwards to dealer
- `receive()`: Receives from dealer, extracts source identity and AttrID, completes transaction

**Asynchronous Methods**:
- `sendAsync()`: Returns `std::future<TransportError>`
- `receiveAsync()`: Returns `std::future<std::pair<TransportError, Frame>>`
- `sendAsyncWithRouting()`: Returns `std::future<TransportError>` while capturing destination identity
- `receiveAsyncWithRouting()`: Returns `std::future<std::tuple<TransportError, std::string, Frame>>` with source identity
- `sendRawAsync()`/`sendRawAsyncWithRouting()`: Take ownership of the payload buffer (moved `std::vector<uint8_t>`) before dispatching to the thread pool

**Message Formats**:
- Without routing: `[delimiter][data]` (2 parts)
- With routing: `[dest_identity][delimiter][data]` (3 parts)

**Thread Safety**:
- All methods lock `mutex_` before accessing `dealer_` or internal state
- Identity changes require lock

#### Usage Example

```cpp
auto tracker = std::make_shared<TransactionTracker>();
TransactionalDealer dealer(tracker);
dealer.setIdentity("worker-001");
dealer.connect("tcp://127.0.0.1:5555");

// Synchronous - send to router, no routing
auto request = MessageBuilder::request(0x10, 0x3000, 7, 1).build();
dealer.send(request);

// Synchronous - send with routing
dealer.send("target-node", request);

// Asynchronous - receive with routing
auto future = dealer.receiveAsyncWithRouting(5000);
auto [error, sourceId, response] = future.get();
```

---

### TransactionalRouter

**Purpose**: Thread-safe wrapper for ZMQRouter with transaction tracking (frontend only)

**Thread Safety**: Thread-safe (internal mutex)

**Pattern**: Asynchronous server with identity-based routing (ROUTER socket)

**Scope**: Frontend-only (does not manage backend clients/dealers)

#### Public Interface

```cpp
class TransactionalRouter {
public:
    explicit TransactionalRouter(std::shared_ptr<TransactionTracker> tracker);
    
    // Binding (thread-safe)
    TransportError bind(const std::string &endpoint);
    
    // Synchronous Frame API - Source Identity Only (thread-safe)
    TransportError receive(std::string &sourceIdentity, Frame &frame, 
                           int timeoutMs = -1);
    TransportError send(const std::string &clientIdentity, const Frame &frame);
    
    // Synchronous Frame API - Source + Destination (thread-safe)
    TransportError receive(std::string &sourceIdentity, 
                           std::string &destinationIdentity,
                           Frame &frame, int timeoutMs = -1);
    TransportError send(const std::string &clientIdentity,
                        const std::string &sourceIdentity,
                        const Frame &frame);
    
    // Synchronous Raw API (thread-safe)
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
    
    // Asynchronous Frame API - Source Identity Only (thread-safe)
    std::future<std::tuple<TransportError, std::string, Frame>> receiveAsync(
        int timeoutMs = -1);
    std::future<TransportError> sendAsync(const std::string &clientIdentity,
                                          const Frame &frame);
    
    // Asynchronous Frame API - Source + Destination (thread-safe)
    std::future<std::tuple<TransportError, std::string, std::string, Frame>>
        receiveAsyncWithRouting(int timeoutMs = -1);
    std::future<TransportError> sendAsyncWithRouting(
        const std::string &clientIdentity,
        const std::string &sourceIdentity,
        const Frame &frame);
    
    // Asynchronous Raw API (thread-safe)
    std::future<std::tuple<TransportError, std::vector<uint8_t>, std::vector<uint8_t>>>
        receiveRawAsync(size_t maxSize);
    std::future<std::tuple<TransportError, std::vector<uint8_t>, std::vector<uint8_t>,
                           std::vector<uint8_t>>>
        receiveRawAsyncWithRouting(size_t maxSize);
    std::future<TransportError> sendRawAsync(
        const std::vector<uint8_t> &clientIdentity, std::vector<uint8_t> data);
    std::future<TransportError> sendRawAsyncWithRouting(
        const std::vector<uint8_t> &clientIdentity,
        const std::vector<uint8_t> &sourceIdentity,
        std::vector<uint8_t> data);

private:
    ZMQRouter router_;
    std::shared_ptr<TransactionTracker> tracker_;
    mutable std::mutex mutex_;
};
```

#### Behavior

**Synchronous Methods**:
- `receive()`: Receives from router, extracts identities and AttrID, registers/completes transaction
- `send()`: Extracts AttrID, registers/completes transaction, forwards to router

**Asynchronous Methods**:
- `receiveAsync()`: Returns `std::future<std::tuple<TransportError, std::string, Frame>>`
- `sendAsync()`: Returns `std::future<TransportError>`
- `receiveAsyncWithRouting()`: Returns `std::future<std::tuple<TransportError, std::string, std::string, Frame>>`
- `sendAsyncWithRouting()`: Returns `std::future<TransportError>` while preserving both client and source identities
- `sendRawAsync()`/`sendRawAsyncWithRouting()`: Take ownership of payload buffers via movable `std::vector<uint8_t>`

**Message Formats**:
- 3-part (source only): `[dealer_identity][delimiter][data]`
- 4-part (source + dest): `[dealer_identity][dest_identity][delimiter][data]`

**Thread Safety**:
- All methods lock `mutex_` before accessing `router_` or internal state
- Transaction operations protected by tracker's internal mutex

#### Usage Example

```cpp
auto tracker = std::make_shared<TransactionTracker>();
TransactionalRouter router(tracker);
router.bind("tcp://0.0.0.0:5555");

// Synchronous - receive and send back
std::string dealerId;
Frame request;
router.receive(dealerId, request);

auto response = MessageBuilder::response(0x30, 0x3000, 1, 1).build();
router.send(dealerId, response);

// Asynchronous - receive with routing
auto future = router.receiveAsyncWithRouting(5000);
auto [error, sourceId, destId, request] = future.get();

// Forward to destination
router.sendAsyncWithRouting(destId, sourceId, response);
```

---

## Transaction Lifecycle

### 1. Client Pattern (REQ-REP)

```
Client (REQ)                    Server (REP)
   │                               │
   ├─ send(frame)                  │
   │  └─ AttrID=123 registered     │
   ├────────────────────────────>  │
   │                               ├─ receive(frame)
   │                               ├─ process
   │                               ├─ send(response)
   │  <────────────────────────────┤
   ├─ receive(frame)               │
   │  └─ AttrID=123 completed      │
   │                               │
```

### 2. Router-Dealer Pattern

```
Dealer (DEALER)             Router (ROUTER)
   │                            │
   ├─ send("target", frame)     │
   │  └─ AttrID=456 registered  │
   │     (dest="target")        │
   ├─────────────────────────>  │
   │                            ├─ receive(src, dest, frame)
   │                            │  └─ src="dealer", dest="target"
   │                            │     AttrID=456 tracked
   │                            │
   │                            ├─ forward to "target"...
   │                            │
   │  <─────────────────────────┤─ send("dealer", "target", response)
   ├─ receive(src, frame)       │
   │  └─ src="target"           │
   │     AttrID=456 completed   │
```

---

## API Summary Table

### TransactionalClient

| Method | Return Type | Thread-Safe | Async | Matches Transport API |
|--------|-------------|-------------|-------|----------------------|
| `connect()` | `TransportError` | ✓ | No | ✓ ZMQClient |
| `send(Frame)` | `TransportError` | ✓ | No | ✓ ZMQClient |
| `receive(Frame, timeout)` | `TransportError` | ✓ | No | ✓ ZMQClient |
| `sendRaw(data, size)` | `TransportError` | ✓ | No | ✓ ZMQClient |
| `receiveRaw(buffer, maxSize)` | `std::ptrdiff_t` | ✓ | No | ✓ ZMQClient |
| `sendAsync(Frame)` | `std::future<TransportError>` | ✓ | Yes | ✓ + Async |
| `receiveAsync(timeout)` | `std::future<std::pair<TransportError, Frame>>` | ✓ | Yes | ✓ + Async |
| `sendRawAsync(vector)` | `std::future<TransportError>` | ✓ | Yes | ✓ + Async |
| `receiveRawAsync(maxSize)` | `std::future<std::pair<std::ptrdiff_t, std::vector<uint8_t>>>` | ✓ | Yes | ✓ + Async |

### TransactionalDealer

| Method | Return Type | Thread-Safe | Async | Matches Transport API |
|--------|-------------|-------------|-------|----------------------|
| `setIdentity()` | `TransportError` | ✓ | No | ✓ ZMQDealer |
| `connect()` | `TransportError` | ✓ | No | ✓ ZMQDealer |
| `getIdentity()` | `const string&` | ✓ | No | ✓ ZMQDealer |
| `send(Frame)` | `TransportError` | ✓ | No | ✓ ZMQDealer |
| `send(dest, Frame)` | `TransportError` | ✓ | No | ✓ ZMQDealer |
| `receive(Frame, timeout)` | `TransportError` | ✓ | No | ✓ ZMQDealer |
| `receive(src, Frame, timeout)` | `TransportError` | ✓ | No | ✓ ZMQDealer |
| `sendRaw(...)` | `TransportError` | ✓ | No | ✓ ZMQDealer |
| `receiveRaw(...)` | `std::ptrdiff_t` | ✓ | No | ✓ ZMQDealer |
| `sendAsync(Frame)` | `std::future<TransportError>` | ✓ | Yes | ✓ + Async |
| `receiveAsync(timeout)` | `std::future<std::pair<TransportError, Frame>>` | ✓ | Yes | ✓ + Async |
| `sendAsyncWithRouting(dest, Frame)` | `std::future<TransportError>` | ✓ | Yes | ✓ + Async |
| `receiveAsyncWithRouting(timeout)` | `std::future<std::tuple<TransportError, std::string, Frame>>` | ✓ | Yes | ✓ + Async |
| `sendRawAsync(vector)` | `std::future<TransportError>` | ✓ | Yes | ✓ + Async |
| `receiveRawAsync(maxSize)` | `std::future<std::pair<std::ptrdiff_t, std::vector<uint8_t>>>` | ✓ | Yes | ✓ + Async |
| `sendRawAsyncWithRouting(dest, vector)` | `std::future<TransportError>` | ✓ | Yes | ✓ + Async |
| `receiveRawAsyncWithRouting(maxSize)` | `std::future<std::tuple<std::ptrdiff_t, std::string, std::vector<uint8_t>>>` | ✓ | Yes | ✓ + Async |

### TransactionalRouter

| Method | Return Type | Thread-Safe | Async | Matches Transport API |
|--------|-------------|-------------|-------|----------------------|
| `bind()` | `TransportError` | ✓ | No | ✓ ZMQRouter |
| `receive(src, Frame, timeout)` | `TransportError` | ✓ | No | ✓ ZMQRouter |
| `receive(src, dest, Frame, timeout)` | `TransportError` | ✓ | No | ✓ ZMQRouter |
| `send(clientId, Frame)` | `TransportError` | ✓ | No | ✓ ZMQRouter |
| `send(clientId, srcId, Frame)` | `TransportError` | ✓ | No | ✓ ZMQRouter |
| `sendRaw(...)` | `TransportError` | ✓ | No | ✓ ZMQRouter |
| `receiveRaw(...)` | `std::ptrdiff_t` | ✓ | No | ✓ ZMQRouter |
| `receiveAsync(timeout)` | `std::future<std::tuple<TransportError, std::string, Frame>>` | ✓ | Yes | ✓ + Async |
| `sendAsync(clientId, Frame)` | `std::future<TransportError>` | ✓ | Yes | ✓ + Async |
| `receiveAsyncWithRouting(timeout)` | `std::future<std::tuple<TransportError, std::string, std::string, Frame>>` | ✓ | Yes | ✓ + Async |
| `sendAsyncWithRouting(clientId, srcId, Frame)` | `std::future<TransportError>` | ✓ | Yes | ✓ + Async |
| `sendRawAsync(clientId, vector)` | `std::future<TransportError>` | ✓ | Yes | ✓ + Async |
| `receiveRawAsync(maxSize)` | `std::future<std::tuple<TransportError, std::vector<uint8_t>, std::vector<uint8_t>>>` | ✓ | Yes | ✓ + Async |
| `sendRawAsyncWithRouting(clientId, srcId, vector)` | `std::future<TransportError>` | ✓ | Yes | ✓ + Async |
| `receiveRawAsyncWithRouting(maxSize)` | `std::future<std::tuple<TransportError, std::vector<uint8_t>, std::vector<uint8_t>, std::vector<uint8_t>>>` | ✓ | Yes | ✓ + Async |

---

## Implementation Considerations

### 1. Thread Pool for Async Operations

```cpp
// Internal thread pool for async operations
class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());
    
    template<typename F>
    auto submit(F&& func) -> std::future<decltype(func())>;
    
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

// Shared thread pool instance
inline ThreadPool& getThreadPool() {
    static ThreadPool pool(4);  // Configurable
    return pool;
}
```

### 2. Mutex Granularity

- **Coarse-grained locking**: Single mutex per wrapper instance
- Locks held during:
  - Transaction registration/completion
  - Transport method calls (send/receive)
  - Internal state access
- Locks released during:
  - Async task execution (state captured beforehand)

### 3. Async Raw Buffer Ownership

- All `sendRawAsync` variants take an owning `std::vector<uint8_t>` parameter
- Payload buffers are moved into the async task before the thread pool dispatch
- Callers can release or reuse their storage immediately after invoking the async API

### 4. Transaction ID Extraction

```cpp
// Helper function to extract AttrID from Frame
inline std::optional<uint16_t> extractTransactionId(const Frame &frame) {
    return frame.header.attrId != 0 
        ? std::optional<uint16_t>(frame.header.attrId) 
        : std::nullopt;
}
```

### 5. Error Handling

- Transport errors propagated directly to caller
- Transaction operations fail silently (log warnings)
- Invalid transaction IDs do not block I/O

### 6. Move Semantics

All wrapper classes support move operations:
```cpp
TransactionalClient client1 = std::move(client2);  // Efficient
```

---

## Usage Patterns

### Pattern 1: Simple Request-Reply (Synchronous)

```cpp
auto tracker = std::make_shared<TransactionTracker>();

// Client
TransactionalClient client(tracker);
client.connect("tcp://127.0.0.1:5555");

auto request = MessageBuilder::request(0x10, 0x3000, 7, 1).build();
client.send(request);

Frame response;
client.receive(response, 5000);
```

### Pattern 2: Async Request-Reply

```cpp
TransactionalClient client(tracker);
client.connect("tcp://127.0.0.1:5555");

// Fire request asynchronously
auto sendFuture = client.sendAsync(request);
auto recvFuture = client.receiveAsync(5000);

// Do other work...

// Wait for response
auto [error, response] = recvFuture.get();
if (error == TransportError::None) {
    // Process response
}
```

### Pattern 3: Broker with Router + Dealer (Frontend-only)

```cpp
auto tracker = std::make_shared<TransactionTracker>();

// Frontend router (receives from external clients)
TransactionalRouter frontend(tracker);
frontend.bind("tcp://0.0.0.0:5555");

// Backend dealer (forwards to workers)
TransactionalDealer backend(tracker);
backend.setIdentity("broker");
backend.connect("tcp://127.0.0.1:5556");

// Routing loop (single thread or event-driven)
while (true) {
    // Receive from frontend
    std::string clientId, workerId;
    Frame request;
    if (frontend.receive(clientId, workerId, request) == TransportError::None) {
        // Forward to backend with routing info
        backend.send(workerId, request);
    }
    
    // Receive from backend
    std::string workerSrc;
    Frame response;
    if (backend.receive(workerSrc, response, 0) == TransportError::None) {
        // Extract original client ID from response routing
        // Forward back to frontend
        frontend.send(clientId, workerSrc, response);
    }
}
```

### Pattern 4: Multi-threaded Async Broker

```cpp
auto tracker = std::make_shared<TransactionTracker>();
TransactionalRouter frontend(tracker);
frontend.bind("tcp://0.0.0.0:5555");

// Thread 1: Handle incoming requests
std::thread frontendThread([&]() {
    while (true) {
        auto future = frontend.receiveAsyncWithRouting(1000);
        auto [error, clientId, workerId, request] = future.get();
        
        if (error == TransportError::None) {
            // Route request (application-specific logic)
            routeRequest(clientId, workerId, request);
        }
    }
});

// Thread 2: Handle outgoing responses
std::thread backendThread([&]() {
    while (true) {
        // Check for responses to send back
        auto responseOpt = getNextResponse();
        if (responseOpt) {
            frontend.sendAsyncWithRouting(responseOpt->clientId,
                                          responseOpt->workerId,
                                          responseOpt->frame);
        }
    }
});
```

---

## Testing Strategy

### Unit Tests
- TransactionTracker: Thread safety, ID generation, timeout detection
- Each wrapper: API parity with transport layer, transaction tracking

### Integration Tests
- Client-Server round-trip with transaction completion
- Router-Dealer routing with identity preservation
- Async operations with concurrent calls

### Stress Tests
- High-frequency transactions (10k+ transactions/sec)
- Multi-threaded concurrent access (10+ threads)
- Transaction timeout cleanup under load

---

## Future Enhancements

### 1. Callback-based Async API
```cpp
client.receiveAsync([](TransportError error, Frame frame) {
    // Handle response
});
```

### 2. Transaction Correlation
```cpp
auto correlationId = tracker->getCorrelation(requestId, responseId);
```

### 3. Transaction Statistics
```cpp
struct Stats {
    size_t totalTransactions;
    size_t completedTransactions;
    size_t timedOutTransactions;
    double avgRoundTripMs;
};
```

### 4. Persistent Transaction Log
Store transaction history for debugging and auditing.

---

## References

- [transport_api.md](transport_api.md) - Base transport layer API specification
- [router_dealer_api.md](router_dealer_api.md) - Router/Dealer pattern details
- ZeroMQ Guide: [https://zguide.zeromq.org/](https://zguide.zeromq.org/)
