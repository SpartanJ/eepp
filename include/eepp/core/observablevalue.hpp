#ifndef EE_CORE_OBSERVABLEVALUE_HPP
#define EE_CORE_OBSERVABLEVALUE_HPP

#include <algorithm>
#include <eepp/config.hpp>
#include <eepp/core/small_vector.hpp>
#include <functional>
#include <memory>
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
		};
		using Observers = SmallVector<Observer, 4>;

		explicit State( T value ) : value( std::move( value ) ) {}

		void set( const T& newValue ) {
			if ( value == newValue )
				return;
			value = newValue;
			notify();
		}

		void set( T&& newValue ) {
			if ( value == newValue )
				return;
			value = std::move( newValue );
			notify();
		}

		void notify() {
			auto snapshot = observers;
			for ( const auto& observer : snapshot )
				observer.callback( value );
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
			return observer != observers.end() && observer->id == id;
		}

		void remove( Uint32 id ) {
			auto observer = find( id );
			if ( observer != observers.end() && observer->id == id )
				observers.erase( observer );
		}

		T value;
		Uint32 nextId{ 0 };
		Observers observers;
	};

  public:
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

	/** @brief Non-owning, lifetime-safe access used by adapters such as UIValueBinding. */
	class WeakHandle {
	  public:
		WeakHandle() = default;

		bool set( const T& value ) const {
			if ( auto state = mState.lock() ) {
				state->set( value );
				return true;
			}
			return false;
		}

		bool set( T&& value ) const {
			if ( auto state = mState.lock() ) {
				state->set( std::move( value ) );
				return true;
			}
			return false;
		}

		explicit operator bool() const { return !mState.expired(); }

	  private:
		friend class ObservableValue<T>;
		explicit WeakHandle( const std::shared_ptr<State>& state ) : mState( state ) {}
		std::weak_ptr<State> mState;
	};

	ObservableValue() : mState( std::make_shared<State>( T{} ) ) {}
	explicit ObservableValue( T value ) : mState( std::make_shared<State>( std::move( value ) ) ) {}
	ObservableValue( const ObservableValue& ) = delete;
	ObservableValue& operator=( const ObservableValue& ) = delete;
	ObservableValue( ObservableValue&& ) noexcept = default;
	ObservableValue& operator=( ObservableValue&& ) noexcept = default;

	const T& get() const { return mState->value; }
	void set( const T& value ) {
		auto state = mState;
		state->set( value );
	}
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

	Connection observe( Callback callback ) {
		auto id = ++mState->nextId;
		mState->observers.emplace_back( typename State::Observer{ id, std::move( callback ) } );
		return Connection( mState, id );
	}

	WeakHandle weakHandle() const { return WeakHandle( mState ); }

  private:
	std::shared_ptr<State> mState;
};

} // namespace EE

#endif
