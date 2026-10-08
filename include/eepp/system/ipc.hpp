#ifndef EE_SYSTEM_IPC_HPP
#define EE_SYSTEM_IPC_HPP

#include <cstddef>
#include <eepp/core/noncopyable.hpp>
#include <eepp/system/time.hpp>
#include <functional>
#include <memory>
#include <string_view>

namespace EE { namespace System {

/** A named, local, same-user, message-oriented inter-process channel.
 *
 * listen() starts a receiver thread. Message callbacks execute on that thread and callers are
 * responsible for dispatching to another thread when required. Each send() creates one connection,
 * transfers one complete framed message, and disconnects.
 */
class EE_API IPC : NonCopyable {
  public:
	/** Result of an IPC operation. Native error codes are mapped to this portable set. */
	enum class Status {
		Done,			 ///< The operation completed successfully.
		NotFound,		 ///< No listener currently owns the destination endpoint.
		AlreadyExists,	 ///< Another listener already owns the endpoint.
		Disconnected,	 ///< The connection closed before the complete message was transferred.
		InvalidEndpoint, ///< The logical endpoint or payload pointer is invalid.
		MessageTooLarge, ///< The payload exceeds MaxMessageSize.
		Timeout,		 ///< The destination did not become available before the timeout.
		Error			 ///< An otherwise unmapped native transport error occurred.
	};

	/** Callback invoked for each complete message received by a listener.
	 *
	 * The callback runs on the listener's worker thread. @p data remains valid only for the
	 * duration of the callback and may contain embedded NUL bytes. For an empty message, @p data is
	 * still a valid pointer and @p size is zero.
	 */
	using MessageFn = std::function<void( const void*, std::size_t )>;

	/** Maximum accepted payload size, excluding the transport frame header. */
	static constexpr std::size_t MaxMessageSize = 16 * 1024 * 1024;

	/** Constructs a closed IPC listener. */
	IPC();

	/** Stops the listener, waits for its worker thread, and releases its native endpoint. */
	~IPC();

	/** Starts listening on a logical endpoint.
	 *
	 * The endpoint is mapped synchronously to a platform-native, same-user local IPC address; the
	 * borrowed logical name is not retained. Only one IPC instance can listen on a given endpoint
	 * at a time. A successful call starts one worker thread and invokes @p callback once for every
	 * complete framed message. Calling listen() on an instance that is already listening closes its
	 * current endpoint first.
	 *
	 * @param endpoint Borrowed logical endpoint name. Native paths or handles are not exposed.
	 * @param callback Function invoked on the IPC worker thread for each complete message.
	 * @return Done on success, AlreadyExists when the endpoint is owned by another listener,
	 * InvalidEndpoint for an empty/invalid endpoint or callback, or Error on a native failure.
	 */
	Status listen( std::string_view endpoint, MessageFn callback );

	/** Stops listening and releases the endpoint.
	 *
	 * This function interrupts pending native waits, joins the worker thread, and guarantees that
	 * the callback will not run after it returns. Repeated calls are safe.
	 */
	void close();

	/** @return Whether this instance currently owns a listening endpoint. */
	bool isListening() const;

	/** Sends one binary message to a logical endpoint.
	 *
	 * A successful call creates a connection, writes exactly one versioned frame containing @p size
	 * bytes, and disconnects. Embedded NUL bytes and zero-length messages are supported. The
	 * timeout applies while establishing or waiting for the destination connection.
	 *
	 * @param endpoint Borrowed logical destination endpoint name.
	 * @param data Payload bytes. May be null only when @p size is zero.
	 * @param size Payload size in bytes, up to MaxMessageSize.
	 * @param timeout Maximum time to wait for the destination connection.
	 * @return A portable status describing delivery or connection failure.
	 */
	static Status send( std::string_view endpoint, const void* data, std::size_t size,
						Time timeout = Seconds( 2 ) );

	/** Sends one string-view payload to a logical endpoint.
	 *
	 * The complete view is sent as opaque bytes; embedded NUL bytes are preserved and no terminator
	 * is appended.
	 *
	 * @param endpoint Borrowed logical destination endpoint name.
	 * @param message Borrowed payload bytes.
	 * @param timeout Maximum time to wait for the destination connection.
	 * @return A portable status describing delivery or connection failure.
	 */
	static Status send( std::string_view endpoint, std::string_view message,
						Time timeout = Seconds( 2 ) );

  private:
	class Impl;
	std::unique_ptr<Impl> mImpl;
};

}} // namespace EE::System

#endif
