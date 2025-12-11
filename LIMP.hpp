/*
==============================================================================
Lightweight Industrial Messaging Protocol (LIMP)
Version: 0.1
Author: Forrest Long
==============================================================================
1. Node Definitions (NodeID)
-----------------------------
HMI          = 0x0010
Server       = 0x0020
PLC          = 0x0030
Alarm        = 0x0040
Logger       = 0x0050

// Reserved Ranges
ProtocolBase = 0x0001 - 0x6FFF  (core protocol assignments)
VendorBase   = 0x7000 - 0x7FFF  (vendor extensions)
UserBase     = 0x8000 - 0xFFFE  (site-specific use)
Broadcast    = 0xFFFF           (all nodes; senders SHOULD rate-limit)

------------------------------------------------------------------------------
2. Message Types (MsgType)
---------------------------
REQUEST       = 0x01
RESPONSE      = 0x02
EVENT         = 0x03
ERROR         = 0x04
SUBSCRIBE     = 0x05
UNSUBSCRIBE   = 0x06
ACK           = 0x07  (acknowledgment for optional reliability)

------------------------------------------------------------------------------
3. Class / Object Definitions (ClassID)
----------------------------------------
System         = 0x1000
IO             = 0x2000
Tag            = 0x3000
Motion         = 0x4000
AlarmObject    = 0x5000
LoggerObject   = 0x6000

// Reserved Ranges
ProtocolBase   = 0x1000 - 0x6FFF  (core protocol classes)
VendorBase     = 0x7000 - 0x7FFF  (vendor-specific classes)
UserBase       = 0x8000 - 0xFFFF  (site-specific classes)

------------------------------------------------------------------------------
4. Instance Definitions (InstanceID)
-------------------------------------
Valid Range: 0x0000 - 0x7FFF (core spec), 0x8000 - 0xFFFF (user/vendor)

// Example for Tag class
Tag0           = 0x0000
Tag1           = 0x0001
Tag2           = 0x0002
...
TagN           = 0x7FFF

// Example for Motion class
Motor0         = 0x0000
Motor1         = 0x0001
...

------------------------------------------------------------------------------
5. Attribute Definitions (AttrID)
---------------------------------
// For Tag class (valid PayloadTypeID shown)
Value          = 0x0001  (FLOAT32 | FLOAT64 | UINT32)
Quality        = 0x0002  (UINT8: 0=Bad, 1=Good, 2=Uncertain)
Timestamp      = 0x0003  (UINT64 epoch ms, see note in section 7)

// For Motion class
Position       = 0x0001  (FLOAT32 | FLOAT64)
Speed          = 0x0002  (FLOAT32 | FLOAT64)
Torque         = 0x0003  (FLOAT32)

// For AlarmObject
Active         = 0x0001  (UINT8: 0=Inactive, 1=Active)
Severity       = 0x0002  (UINT8: 0=Info, 1=Warning, 2=Critical)
Message        = 0x0003  (STRING)

------------------------------------------------------------------------------
6. Error Codes (Payload in ERROR MsgType)
-----------------------------------------
InvalidClass       = 0x01
InvalidInstance    = 0x02
InvalidAttribute   = 0x03
PermissionDenied   = 0x04
BadPayload         = 0x05
InternalError      = 0x06
UnsupportedVersion = 0x07
InvalidFlags       = 0x08

Error Response Protocol:
- ERROR messages MUST echo the ClassID, InstanceID, and AttrID from the
  original request that caused the error
- SrcNodeID/DstNodeID are swapped (responder becomes source)
- PayloadTypeID should be UINT8 (0x01) containing the error code
- For protocol-level errors (version, flags), ClassID/InstanceID/AttrID = 0

------------------------------------------------------------------------------
7. Payload Types (PayloadTypeID)
---------------------------------
NONE        = 0x00  (no payload; PayloadLen MUST be 0)
UINT8       = 0x01  (PayloadLen = 1)
UINT16      = 0x02  (PayloadLen = 2)
UINT32      = 0x03  (PayloadLen = 4)
UINT64      = 0x04  (PayloadLen = 8)
FLOAT32     = 0x05  (PayloadLen = 4)
FLOAT64     = 0x06  (PayloadLen = 8)
STRING      = 0x07  (UTF-8, PayloadLen = actual bytes, null-terminator optional)
OPAQUE      = 0x08  (binary blob, PayloadLen = actual bytes)

Note: All multi-byte values use big-endian byte order (network byte order).
Receivers MUST validate PayloadLen matches expected size for fixed-length types.
For STRING: PayloadLen includes all bytes sent; null terminator (if present) counts
toward PayloadLen. Receivers should handle strings with or without null terminators.

------------------------------------------------------------------------------
8. LIMP Frame Format (Binary)
-----------------------------
Offset  Size  Field
------  ----  -----------------------------
0       1     Version
1       1     MsgType
2       2     SrcNodeID (uint16, big-endian)
4       2     DstNodeID (uint16, big-endian)
6       2     ClassID (uint16, big-endian)
8       2     InstanceID (uint16, big-endian)
10      2     AttrID (uint16, big-endian)
12      1     PayloadTypeID
13      2     PayloadLen (uint16, big-endian; payload bytes only)
15      1     Flags (bit0: CRC present, bits 1-7: reserved, MUST be 0)
16      N     Payload (N = PayloadLen)
16+N   2/0    CRC16-IBM (if Flags bit0 = 1; big-endian)

Frame Size: Minimum 16 bytes (no payload, no CRC), Maximum 65552 bytes
            (16 header + 65534 payload + 2 CRC)

CRC16-IBM: Polynomial 0x8005, initial value 0x0000, no reflection, computed
           over bytes 0-15+N (entire frame except CRC itself)

Version Handling: Receivers MUST reject frames with Version != 0x01 by sending
                  ERROR with InternalError code

Flags Handling: Receivers MUST ignore frames with non-zero reserved bits (1-7)

------------------------------------------------------------------------------
9. Example Request/Response Frames
------------------------------
// 9.1 HMI requests PLC Tag[7].Value
Version:      0x01
MsgType:      0x01 (REQUEST)
SrcNodeID:    0x00 0x10 (HMI, big-endian)
DstNodeID:    0x00 0x30 (PLC, big-endian)
ClassID:      0x30 0x00 (Tag, big-endian)
InstanceID:   0x00 0x07
AttrID:       0x00 0x01 (Value)
PayloadTypeID:0x00 (NONE)
PayloadLen:   0x00 0x00
Flags:        0x00 (no CRC)
Payload:      (none)

// 9.2 PLC responds with float32=123.45
Version:      0x01
MsgType:      0x02 (RESPONSE)
SrcNodeID:    0x00 0x30 (PLC, big-endian)
DstNodeID:    0x00 0x10 (HMI, big-endian)
ClassID:      0x30 0x00
InstanceID:   0x00 0x07
AttrID:       0x00 0x01
PayloadTypeID:0x05 (FLOAT32)
PayloadLen:   0x00 0x04
Flags:        0x01 (CRC present)
Payload:      0x42 0xF6 0xE6 0x66 (BE encoding of 123.45)
CRC16:        0x8A 0x3C (example, BE)

10. Example Error Frame
------------------------
Version:      0x01
MsgType:      0x04 (ERROR)
SrcNodeID:    0x00 0x30 (PLC - responding to HMI request)
DstNodeID:    0x00 0x10 (HMI - original requester)
ClassID:      0x30 0x00 (echoed from request)
InstanceID:   0x00 0x07 (echoed from request)
AttrID:       0x00 0x01 (echoed from request)
PayloadTypeID:0x01 (UINT8)
PayloadLen:   0x00 0x01
Flags:        0x00 (no CRC)
Payload:      0x03 (InvalidAttribute)

------------------------------------------------------------------------------
11. Implementation Notes
------------------------
Transport Layer:
  - LIMP is transport-agnostic; can run over TCP, UDP, serial, or message queues
  - Implementers should define framing/delimiting for stream-based transports
  - Recommended: length-prefix or start/end delimiters for serial/TCP

Request/Response Matching:
  - v0.1 does not include transaction IDs; matching relies on
    (SrcNodeID, DstNodeID, ClassID, InstanceID, AttrID) tuple
  - For concurrent requests to same target, use ACK message type or implement
    at application layer

Timeouts:
  - Recommended REQUEST timeout: 1000-5000ms depending on network
  - Recommended SUBSCRIBE keepalive: 30-60s
  - No protocol-level timeout specification; handle in application layer

SUBSCRIBE/UNSUBSCRIBE Semantics:
  - PayloadTypeID = NONE for these message types
  - Subscriber uses REQUEST addressing; publisher responds with EVENT messages
  - Server maintains subscription list; sends EVENT on value changes
  - UNSUBSCRIBE uses same addressing to cancel subscription

Security:
  - v0.1 includes no authentication or encryption
  - For production: implement TLS at transport layer or add security extension
  - Consider adding signature/HMAC in future versions using reserved flag bits

Fragmentation:
  - v0.1 does not support fragmentation
  - STRING/OPAQUE payloads limited to 65534 bytes
  - For larger data, implement chunking at application layer or use multiple
    attributes (e.g., DataChunk0, DataChunk1, ...)

==============================================================================
End
==============================================================================

*/
#ifndef LIMP_HPP
#define LIMP_HPP
// LIMP Protocol Definitions
#endif // LIMP_HPP