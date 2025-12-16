# Publish-Subscribe Pattern

One-to-many message distribution using ZeroMQ PUB-SUB sockets.

## Examples

### publisher_subscriber.cpp
Combined publisher and subscriber in one program (uses threads).

**Run**:
```bash
./build/examples/03_pubsub/publisher_subscriber
```

### subscriber_only.cpp
Standalone subscriber connecting to publisher.

**Run**:
```bash
./build/examples/03_pubsub/subscriber_only
```

## Pattern
- One-to-many broadcast
- Fire-and-forget (no acknowledgment)
- Topic-based filtering
