#pragma once

/**
 * LIMP - Lightweight Industrial Messaging Protocol
 * C++ Standard Library
 *
 * Main header file - include this to use the LIMP library
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
