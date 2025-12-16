# Router-Dealer Pattern

Asynchronous communication with identity-based routing.

## Examples

### router.cpp
ROUTER socket handling async requests from multiple dealers.

**Run**:
```bash
./build/examples/04_router_dealer/router
```

### dealer.cpp
DEALER socket sending async requests (no strict send-recv order).

**Run** (separate terminal):
```bash
./build/examples/04_router_dealer/dealer
```

## Pattern
- Asynchronous messaging
- N:M communication
- Identity-based routing
- No blocking
