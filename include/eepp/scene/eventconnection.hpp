#ifndef EE_SCENE_EVENTCONNECTION_HPP
#define EE_SCENE_EVENTCONNECTION_HPP

#include <eepp/config.hpp>
#include <eepp/core/small_vector.hpp>
#include <memory>

namespace EE { namespace Scene {

struct EventConnectionState;

/**
 * @brief Move-only handle that disconnects a Node event listener on destruction.
 *
 * EventConnection provides scoped ownership for a listener registered with Node::connect(). If
 * the connection is destroyed first, its listener is removed from the emitter. If the emitter is
 * destroyed first, the connection expires and its eventual destruction is a safe no-op.
 *
 * A connection does not keep its emitting Node alive. Moving a connection transfers listener
 * ownership; copying is disabled so that exactly one handle owns the scoped listener.
 *
 * Emitter destruction silently expires the connection. EventConnection is therefore not a
 * replacement for UIWidget::OnClose: observers that must perform work when a widget closes must
 * still connect to that widget-level notification.
 *
 * Event registration, dispatch, disconnection, and connection destruction must all happen on the
 * emitter's owning thread. EventConnection does not make Node's event registry thread-safe.
 */
class EE_API EventConnection {
  public:
	/** @brief Creates a disconnected handle. */
	EventConnection() = default;

	/** @brief Disconnects the owned listener, if its emitter still exists. */
	~EventConnection();

	/** @brief Transfers listener ownership from @p other. */
	EventConnection( EventConnection&& other ) noexcept;

	/**
	 * @brief Disconnects the currently owned listener and transfers ownership from @p other.
	 */
	EventConnection& operator=( EventConnection&& other ) noexcept;
	EventConnection( const EventConnection& ) = delete;
	EventConnection& operator=( const EventConnection& ) = delete;

	/**
	 * @brief Removes the listener and makes this handle disconnected.
	 *
	 * Calling disconnect() more than once, or after the emitter has been destroyed, is safe.
	 */
	void disconnect();

	/**
	 * @return True while this handle owns a listener and the emitter's event state still exists.
	 */
	explicit operator bool() const;

  private:
	friend class Node;
	EventConnection( std::weak_ptr<EventConnectionState> state, Uint32 eventType,
					 Uint32 callbackId );

	std::weak_ptr<EventConnectionState> mState;
	Uint32 mEventType{ 0 };
	Uint32 mCallbackId{ 0 };
};

/**
 * @brief Owns a small group of event connections and disconnects them together.
 *
 * The first four connections are stored inline. Destroying or clearing the list destroys every
 * contained EventConnection and therefore disconnects all of their listeners. As with an
 * individual EventConnection, the list must be manipulated on the emitters' owning thread.
 */
class EE_API EventConnectionList {
  public:
	EventConnectionList() = default;
	EventConnectionList( EventConnectionList&& ) noexcept = default;
	EventConnectionList& operator=( EventConnectionList&& ) noexcept = default;
	EventConnectionList( const EventConnectionList& ) = delete;
	EventConnectionList& operator=( const EventConnectionList& ) = delete;

	/** @brief Adds a connection by transferring its ownership into this list. */
	EventConnectionList& add( EventConnection connection );

	/** @brief Equivalent to add(). */
	EventConnectionList& operator+=( EventConnection connection );

	/** @brief Disconnects and removes all owned connections. */
	void clear();

	/** @return True if the list owns no connections. */
	bool empty() const;

	/** @return The number of connections owned by the list. */
	size_t size() const;

  private:
	SmallVector<EventConnection, 4> mConnections;
};

}} // namespace EE::Scene

#endif
