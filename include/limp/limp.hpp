#pragma once

/**
 * LIMP - Lightweight Industrial Messaging Protocol
 * C++ Standard Library
 *
 * Main header file - include this to use the LIMP library
 */

/*
==============================================================================
Lightweight Industrial Messaging Protocol (LIMP)
Version: 0.1
Author: Forrest Long
==============================================================================

1. Message Types (MsgType)
---------------------------
REQUEST       = 0x01  - Request data or action from target
RESPONSE      = 0x02  - Response to a REQUEST
EVENT         = 0x03  - Unsolicited event notification
ERROR         = 0x04  - Error response
SUBSCRIBE     = 0x05  - Subscribe to data changes
UNSUBSCRIBE   = 0x06  - Unsubscribe from data changes
ACK           = 0x07  - Acknowledgment

------------------------------------------------------------------------------
2. Payload Types (PayloadTypeID)
---------------------------------
NONE        = 0x00  - No payload (PayloadLen MUST be 0)
UINT8       = 0x01  - 8-bit unsigned integer (PayloadLen = 1)
UINT16      = 0x02  - 16-bit unsigned integer (PayloadLen = 2)
UINT32      = 0x03  - 32-bit unsigned integer (PayloadLen = 4)
UINT64      = 0x04  - 64-bit unsigned integer (PayloadLen = 8)
FLOAT32     = 0x05  - 32-bit IEEE 754 float (PayloadLen = 4)
FLOAT64     = 0x06  - 64-bit IEEE 754 double (PayloadLen = 8)
STRING      = 0x07  - UTF-8 string (variable length)
OPAQUE      = 0x08  - Binary blob (variable length)

Note: All multi-byte values use big-endian byte order (network byte order).

------------------------------------------------------------------------------
3. LIMP Frame Format (Binary Wire Format)
-----------------------------
Offset  Size  Field
------  ----  -----------------------------
0       1     Version (0x01)
1       1     MsgType
2       2     SrcNodeID (uint16, big-endian)
4       2     ClassID (uint16, big-endian)
6       2     InstanceID (uint16, big-endian)
8       2     AttrID (uint16, big-endian)
10      1     PayloadTypeID
11      2     PayloadLen (uint16, big-endian; payload bytes only)
13      1     Flags (bit0: CRC present, bits 1-7: reserved, MUST be 0)
14      N     Payload (N = PayloadLen)
14+N   2/0    CRC16-MODBUS (if Flags bit0 = 1; little-endian)

Frame Size: Minimum 14 bytes (header only), Maximum 65550 bytes

CRC16-MODBUS: Polynomial 0xA001 (reflected), Initial 0xFFFF, LSB-first
              Computed over entire frame except CRC bytes

------------------------------------------------------------------------------
4. Protocol Rules
---------------------------------

Version Handling:
  - Current version: 0x01
  - Receivers MUST reject frames with unsupported versions

Flags Handling:
  - Bit 0: CRC_PRESENT (0=no CRC, 1=CRC appended)
  - Bits 1-7: Reserved, MUST be 0
  - Receivers MUST reject frames with non-zero reserved bits

Error Response Protocol:
  - ERROR messages MUST echo ClassID, InstanceID, and AttrID from original request
  - SrcNodeID identifies the error responder
  - PayloadTypeID typically UINT8 containing application error code
  - For protocol-level errors, ClassID/InstanceID/AttrID may be 0

Request/Response Matching:
  - No built-in transaction IDs
  - Matching relies on (SrcNodeID, ClassID, InstanceID, AttrID) tuple
  - For concurrent requests, implement transaction tracking at application layer

------------------------------------------------------------------------------
5. Transport Layer Notes
------------------------

Transport Agnostic:
  - LIMP is transport-agnostic (TCP, UDP, ZeroMQ, serial, etc.)
  - Stream-based transports need framing/delimiting mechanism
  - Datagram transports: one frame per datagram

Timeouts:
  - No protocol-level timeout specification
  - Recommended REQUEST timeout: 1000-5000ms
  - Handle timeouts at application or transport layer

SUBSCRIBE/UNSUBSCRIBE:
  - SUBSCRIBE: Client requests data change notifications
  - Server responds with EVENT messages on value changes
  - UNSUBSCRIBE: Uses same addressing to cancel subscription
  - Server maintains subscription list

Security:
  - v0.1 has no built-in authentication or encryption
  - For production: use TLS at transport layer
  - Application layer may implement custom security

Fragmentation:
  - Not supported in v0.1
  - STRING/OPAQUE payloads limited to 65534 bytes
  - For larger data: implement chunking at application layer

==============================================================================
Application Layer Responsibilities
==============================================================================

The LIMP protocol defines the wire format and message semantics.
Applications must define:

- Node IDs: Unique identifiers for nodes in the system
- Class IDs: Object classes (e.g., Tag=0x3000, Motor=0x4000)
- Instance IDs: Specific object instances
- Attribute IDs: Properties of objects (e.g., Value=0x0001, Status=0x0002)
- Error Codes: Application-specific errors encoded in ERROR message payload

Refer to application-specific documentation for these definitions.

==============================================================================
End
==============================================================================

*/

#include "limp/types.hpp"
#include "limp/frame.hpp"
#include "limp/message.hpp"
#include "limp/transport.hpp"
#include "limp/utils.hpp"
#include "limp/crc.hpp"

namespace limp
{

    // Library version
    constexpr const char *VERSION = "0.1.0";

} // namespace limp
