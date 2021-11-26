#pragma once
#include <memory>
#include <string>

namespace asyncio {

using StringPtr = std::shared_ptr<std::string>;

class Transport;
using TransportPtr = std::shared_ptr<Transport>;

class Protocol;
using ProtocolPtr = std::shared_ptr<Protocol>;

class Listener;
using ListenerPtr = std::unique_ptr<Listener>;

class DelayTimer;
using DelayTimerPtr = std::unique_ptr<DelayTimer>;

} // namespace asyncio
