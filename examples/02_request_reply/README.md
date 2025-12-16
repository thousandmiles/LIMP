# Request-Reply Pattern

Client-server communication using ZeroMQ REQ-REP sockets.

## Examples

### server.cpp
REP server listening on `tcp://0.0.0.0:5555`.

**Run**:
```bash
./build/examples/02_request_reply/server
```

### client.cpp
REQ client connecting to `tcp://127.0.0.1:5555`.

**Run** (separate terminal):
```bash
./build/examples/02_request_reply/client
```

## Pattern
- Synchronous request-response
- Client blocks until reply
- 1:1 communication
