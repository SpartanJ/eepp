#ifndef EE_CORE_OBSERVABLEVALUE_HPP
#define EE_CORE_OBSERVABLEVALUE_HPP

#include <algorithm>
#include <eepp/config.hpp>
#include <eepp/core/debug.hpp>
#include <eepp/core/small_vector.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <utility>

namespace EE {

/**
 * @brief Owns a value and notifies scoped observers after the value changes.
 *
 * ObservableValue is a deliberately small synchronous primitive. Assignment and set() notify
 * observers immediately on the calling thread. Observer callbacks use snapshot semantics: changes
 * to the observer list during a notification take effect on the next notification.
 *
 * The class is non-copyable. Moving it transfers the value and its existing observers, allowing
 * handles and UI bindings to keep observing the moved-to instance. ObservableValue and all of its
 * connections must be used from a single owning thread.
 *
 * Use ObservableValue for model or application state whose observers are not known by the model.
 * A configuration object, for example, can publish changes without depending on UIWidget; a live
 * UI may attach with UIValueBinding and disappear safely later.
 *
 * @code
 * struct ApplicationConfig {
 *     ObservableValue<bool> showLineNumbers{ true };
 * };
 * ApplicationConfig config;
 * auto connection = config.showLineNumbers.observe(
 *     []( bool enabled ) { updateEditorPolicy( enabled ); } );
 * config.showLineNumbers = false;
 * @endcode
 */
template <typename T> class ObservableValue {
  private:
	struct State {
		using Callback = std::function<void( const T& )>;
		struct Observer {
			Uint32 id;
			Callback callback;
			bool connected{ true };
		};
		using Observers = SmallVector<Observer, 4>;

		explicit State( T value ) : value( std::move( value ) ) {}

		void set( const T& newValue ) { setImpl( newValue ); }

		void set( T&& newValue ) { setImpl( std::move( newValue ) ); }

		template <typename U> void setImpl( U&& newValue ) {
			if ( notifying ) {
				if ( value == newValue ) {
					pendingValue.reset();
				} else if ( !pendingValue || *pendingValue != newValue ) {
					pendingValue = std::forward<U>( newValue );
				}
				return;
			}
			if ( value == newValue )
				return;

			value = std::forward<U>( newValue );
			notifying = true;
			Uint32 notificationCount = 0;
			do {
				// Keep the callback objects in their stable observer slots while invoking them. New
				// observers go into pendingObservers so growing the container cannot relocate a
				// std::function that is currently executing. Disconnected observers are tombstoned
				// until this pass ends, preserving snapshot semantics without copying callbacks.
				const std::size_t observerCount = observers.size();
				for ( std::size_t i = 0; i < observerCount; ++i )
					// Disconnections made during this delivery take effect on the next one.
					// The observer remains in place so callbacks are never copied here.
					observers[i].callback( value );
				observers.erase( std::remove_if( observers.begin(), observers.end(),
												 []( const Observer& observer ) {
													 return !observer.connected;
												 } ),
								 observers.end() );
				for ( auto& observer : pendingObservers )
					observers.emplace_back( std::move( observer ) );
				pendingObservers.clear();
				if ( !pendingValue )
					break;
				value = std::move( *pendingValue );
				pendingValue.reset();
				// A bounded drain turns accidental observer cycles into a clear debug failure
				// instead of unbounded recursion (or an infinite release-build loop).
				if ( ++notificationCount == MaxReentrantNotifications ) {
					eeASSERTM( false, "ObservableValue observer cycle detected" );
					break;
				}
			} while ( true );
			notifying = false;
			pendingValue.reset();
		}

		typename Observers::iterator find( Uint32 id ) {
			return std::lower_bound( observers.begin(), observers.end(), id,
									 []( const Observer& observer, Uint32 observerId ) {
										 return observer.id < observerId;
									 } );
		}

		typename Observers::const_iterator find( Uint32 id ) const {
			return std::lower_bound( observers.begin(), observers.end(), id,
									 []( const Observer& observer, Uint32 observerId ) {
										 return observer.id < observerId;
									 } );
		}

		bool contains( Uint32 id ) const {
			auto observer = find( id );
			if ( observer != observers.end() && observer->id == id )
				return observer->connected;
			auto pending = std::lower_bound(
				pendingObservers.begin(), pendingObservers.end(), id,
				[]( const Observer& item, Uint32 observerId ) { return item.id < observerId; } );
			return pending != pendingObservers.end() && pending->id == id && pending->connected;
		}

		void remove( Uint32 id ) {
			auto observer = find( id );
			if ( observer != observers.end() && observer->id == id ) {
				if ( notifying )
					observer->connected = false;
				else
					observers.erase( observer );
				return;
			}
			auto pending = std::lower_bound(
				pendingObservers.begin(), pendingObservers.end(), id,
				[]( const Observer& item, Uint32 observerId ) { return item.id < observerId; } );
			if ( pending != pendingObservers.end() && pending->id == id )
				pendingObservers.erase( pending );
		}

		void add( Uint32 id, Callback callback ) {
			// Appending directly while a callback runs could reallocate observers and destroy the
			// executing std::function. Pending callbacks become visible on the next delivery.
			auto& destination = notifying ? pendingObservers : observers;
			destination.emplace_back( Observer{ id, std::move( callback ), true } );
		}

		T value;
		static constexpr Uint32 MaxReentrantNotifications = 1024;
		Uint32 nextId{ 0 };
		Observers observers;
		// Separate inline storage avoids both callback copies and heap allocation for the usual
		// case of a few observers added during notification.
		Observers pendingObservers;
		std::optional<T> pendingValue;
		bool notifying{ false };
	};

  public:
	using ValueType = T;
	using Callback = std::function<void( const T& )>;

	/** @brief Move-only scoped ownership of one ObservableValue observer. */
	class Connection {
	  public:
		Connection() = default;
		~Connection() { disconnect(); }
		Connection( const Connection& ) = delete;
		Connection& operator=( const Connection& ) = delete;

		Connection( Connection&& other ) noexcept :
			mState( std::move( other.mState ) ), mId( other.mId ) {
			other.mId = 0;
		}

		Connection& operator=( Connection&& other ) noexcept {
			if ( this != &other ) {
				disconnect();
				mState = std::move( other.mState );
				mId = other.mId;
				other.mId = 0;
			}
			return *this;
		}

		/** @brief Disconnects the observer. Calling this more than once is safe. */
		void disconnect() {
			if ( auto state = mState.lock() )
				state->remove( mId );
			mState.reset();
			mId = 0;
		}

		explicit operator bool() const {
			if ( auto state = mState.lock() )
				return mId != 0 && state->contains( mId );
			return false;
		}

	  private:
		friend class ObservableValue<T>;
		Connection( const std::shared_ptr<State>& state, Uint32 id ) : mState( state ), mId( id ) {}

		std::weak_ptr<State> mState;
		Uint32 mId{ 0 };
	};

	/**
	 * @brief Non-owning, lifetime-safe access used by adapters such as UIValueBinding.
	 *
	 * Operations fail harmlessly after the owning ObservableValue is destroyed. A WeakHandle does
	 * not make cross-thread access safe.
	 */
	class WeakHandle {
	  public:
		WeakHandle() = default;

		/** @return true when the owner still exists and accepted the set operation. */
		bool set( const T& value ) const {
			if ( auto state = mState.lock() ) {
				state->set( value );
				return true;
			}
			return false;
		}

		/** @return true when the owner still exists and accepted the set operation. */
		bool set( T&& value ) const {
			if ( auto state = mState.lock() ) {
				state->set( std::move( value ) );
				return true;
			}
			return false;
		}

		/** @return A copy of the current value, or std::nullopt after the owner expires. */
		std::optional<T> get() const {
			if ( auto state = mState.lock() )
				return state->value;
			return std::nullopt;
		}

		/** @return A scoped observer connection, or an empty connection after owner expiration. */
		Connection observe( Callback callback ) const {
			if ( auto state = mState.lock() ) {
				auto id = ++state->nextId;
				state->add( id, std::move( callback ) );
				return Connection( state, id );
			}
			return {};
		}

		explicit operator bool() const { return !mState.expired(); }

	  private:
		friend class ObservableValue<T>;
		explicit WeakHandle( const std::shared_ptr<State>& state ) : mState( state ) {}
		std::weak_ptr<State> mState;
	};

	/** @brief Creates an observable containing a default-constructed value. */
	ObservableValue() : mState( std::make_shared<State>( T{} ) ) {}

	/** @brief Creates an observable containing @p value. No notification is emitted. */
	explicit ObservableValue( T value ) : mState( std::make_shared<State>( std::move( value ) ) ) {}
	ObservableValue( const ObservableValue& ) = delete;
	ObservableValue& operator=( const ObservableValue& ) = delete;
	ObservableValue( ObservableValue&& ) noexcept = default;
	ObservableValue& operator=( ObservableValue&& ) noexcept = default;

	/** @return A reference to the current value. */
	const T& get() const { return mState->value; }

	/** @brief Replaces the value and synchronously notifies observers when it changed. */
	void set( const T& value ) {
		auto state = mState;
		state->set( value );
	}

	/** @brief Move-replaces the value and synchronously notifies observers when it changed. */
	void set( T&& value ) {
		auto state = mState;
		state->set( std::move( value ) );
	}

	ObservableValue& operator=( const T& value ) {
		set( value );
		return *this;
	}

	ObservableValue& operator=( T&& value ) {
		set( std::move( value ) );
		return *this;
	}

	const T& operator*() const { return get(); }

	const T* operator->() const { return &get(); }

	operator const T&() const { return get(); }

	/**
	 * @brief Observes subsequent value changes.
	 * @return A scoped connection; destroying it disconnects the callback.
	 *
	 * Registration does not invoke @p callback with the current value. Reentrant set() calls are
	 * queued and delivered after the current observer snapshot completes.
	 */
	Connection observe( Callback callback ) {
		auto id = ++mState->nextId;
		mState->add( id, std::move( callback ) );
		return Connection( mState, id );
	}

	/** @return A non-owning handle that expires safely with this observable. */
	WeakHandle weakHandle() const { return WeakHandle( mState ); }

	/** @return The number of currently connected observers. */
	std::size_t observerCount() const { return mState->observers.size(); }

	/** @return Whether observer callbacks are currently being delivered. */
	bool isNotifying() const { return mState->notifying; }

  private:
	std::shared_ptr<State> mState;
};

} // namespace EE

#endif
