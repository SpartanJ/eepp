# WebSocket

`EE::Network::WebSocket` implements RFC 6455 framing and opening handshakes over an owned
connected `TcpSocket`. Transfer an `SSL::SSLSocket` for TLS (`wss`). Construction takes a
`std::unique_ptr<TcpSocket>` and transfers exclusive ownership. Local streams can use the separate
`LocalSocket` API with `WebSocket::Decoder` and `WebSocket::encode()` for framing.
Destroying the WebSocket closes the connection. There is no background thread.

```cpp
using namespace EE::Network;
using namespace EE::System;

auto transport = std::make_unique<TcpSocket>();
if ( transport->connect( IpAddress::LocalHost, 8080, Seconds( 5 ) ) == Socket::Done ) {
    transport->setSendTimeout( Seconds( 5 ) );
    transport->setReceiveTimeout( Seconds( 5 ) );
    WebSocket websocket( std::move( transport ), WebSocket::Role::Client );
    if ( websocket.open( "localhost:8080", "/events" ) == Socket::Done ) {
        websocket.send( "hello" );
        std::string payload;
        WebSocket::Opcode opcode;
        if ( websocket.receive( payload, opcode ) == Socket::Done ) {
            // Text or Binary; ping/pong and peer close are handled internally.
        }
        // Complete the closing handshake: receive() discards data until the peer's close.
        if ( websocket.close() == Socket::Done )
            websocket.receive( payload, opcode );
    }
}
```

Connect and configure an `SSL::SSLSocket` with normal certificate and hostname validation before
transferring it to the same interface. WebSocket does not change TLS policy. The high-level operations
require blocking transports; send/receive timeouts belong to the transport. `close()` sends a close
frame; a later `receive()` discards incoming data until the peer's close frame arrives, then
reports `Disconnected`. Destruction disconnects its owned transport.

For a server, accept an owned `TcpSocket` through `TcpListener`, transfer it into a WebSocket with
`Role::Server`, and call `accept()`. The convenience handshake does not impose authentication or
origin policy. Servers that handle access policy themselves can validate the HTTP request, send
`serverResponse(request)`, and use the independent `Decoder`/`encode` API for subsequent messages.

The default incoming message limit is 8 MiB, configurable in the constructor. Oversized messages,
invalid masking, reserved bits/opcodes, invalid continuation sequences, malformed control frames,
invalid close codes/reasons and invalid UTF-8 text fail decoding. Compression and subprotocol
negotiation are not implemented. Client keys and masks use the OS cryptographic random source.
Custom client headers can supply authorization; reserved handshake and HTTP body-framing fields
cannot be overridden.

`WebSocket::Decoder` is independent of sockets. Feed arbitrary byte chunks to it for nonblocking
servers or stream observers. It assembles fragmented messages, delivers interleaved control frames,
and passes borrowed message views to the callback. Its optional capture predicate can skip data
payload buffering at message boundaries; passive observers can also skip oversized messages.
`takeOversizedMessage()` reports and clears that notification, allowing an observer to settle any
application state it could not inspect. Forwarding of the original stream remains the caller's job.
Endpoint decoders should retain the default behavior of failing on an oversized message.

`SocketSelector::add(socket, Event::Write)` observes writable readiness, and `Event::ReadWrite`
observes both directions. Repeated additions merge interests; `clear()` removes them. The existing
`isReady(socket)` checks read readiness, while the overload accepts a specific event.

Local streams support native POSIX platforms and Windows 10 1803+ with AF_UNIX support.
Paths must fit the OS socket-address limit. Emscripten and unavailable OS facilities return
`Socket::Error`.

The wire format and handshake follow [RFC 6455](https://www.rfc-editor.org/rfc/rfc6455).

`LocalSocket` and `LocalListener` provide filesystem local streams without TCP-only methods or
options. A listener owns and removes the endpoint it successfully bound; it never removes an
existing endpoint during `listen()`. Put it in a private directory. `shutdown()` interrupts a
blocked local stream without releasing the handle; wait for its I/O thread before destruction.
A nonblocking local `connect()` may return `NotReady`; watch write readiness, then call
`finishConnect()` to confirm completion or detect refusal. A full Linux listen queue returns `Error`,
since no connection has started. `Socket::getHandle()` borrows a native handle for OS interoperability;
the socket retains ownership and must outlive native operations.

`StreamRelay<Left, Right>` owns two stream sockets and relays bytes in both directions. Connect or
accept through `left()` and `right()`, then add its interests with `watch(selector)`, wait, and call
`poll(selector)`. It creates no threads. Reads use 64 KiB chunks; each direction queues at most 128 KiB after filtering and stops reading
when the target applies backpressure. The extra space permits a filter to prepend a retained header. An optional filter can inspect, replace or drop a chunk; filters
must bound their own retained data. eterm uses it with TCP and local sockets and keeps its HTTP
authorization and session-selection policy in the application.

A failed or partial WebSocket frame send closes the transport: another frame cannot safely follow
an incomplete frame. HTTP field extraction uses the same validated field parser as `Http::Response`.
