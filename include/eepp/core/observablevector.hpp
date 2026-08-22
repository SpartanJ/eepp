#ifndef EE_CORE_OBSERVABLEVECTOR_HPP
#define EE_CORE_OBSERVABLEVECTOR_HPP

#include <eepp/core/observablevalue.hpp>
#include <vector>

namespace EE {

/**
 * @brief A vector whose explicit mutations can incrementally update attached adapters.
 *
 * Use this when a collection remains live while a view is attached. Immutable option lists and
 * collections already managed by a specialized Model should continue using those simpler models.
 * Notifications are synchronous and the collection and its connections must be used from one
 * owning thread. See the ui_data_collections example for live insertion, updates, and removal.
 */
template <typename T> class ObservableVector {
  public:
	/** @brief The mutation represented by a Change notification. */
	enum class ChangeType { Insert, Remove, Move, Change, Reset };

	/** @brief Whether a Change is emitted immediately before or after its mutation. */
	enum class Phase { Before, After };

	/** @brief Describes one collection mutation for incremental consumers. */
	struct Change {
		ChangeType type;
		Phase phase;
		std::size_t index{ 0 };
		std::size_t count{ 0 };
		std::size_t target{ 0 };
	};
	using ValueType = std::vector<T>;
	using Callback = std::function<void( const Change& )>;

  private:
	struct State {
		struct Observer {
			std::shared_ptr<Callback> callback;
			Uint64 removedGeneration{ 0 };
			Uint32 id;
		};
		std::vector<T> values;
		SmallVector<Observer, 4> observers;
		Uint64 notificationGeneration{ 0 };
		Uint64 activeGeneration{ 0 };
		Uint32 nextId{ 0 };
		Uint32 notificationDepth{ 0 };
		void notify( const Change& change ) {
			// Each nested delivery gets a generation. A connection removed in generation N remains
			// callable by snapshots from generation <= N, but is invisible to later nested or
			// future deliveries. This exactly preserves snapshot behavior without copying
			// std::function.
			const Uint64 parentGeneration = activeGeneration;
			const Uint64 generation = ++notificationGeneration;
			activeGeneration = generation;
			++notificationDepth;
			const std::size_t observerCount = observers.size();
			for ( std::size_t i = 0; i < observerCount; ++i ) {
				auto& observer = observers[i];
				if ( observer.removedGeneration == 0 || observer.removedGeneration >= generation ) {
					// Keep the target alive locally: a callback may grow and reallocate observers
					// or disconnect itself while it is executing. Copying shared_ptr never
					// allocates.
					auto callback = observer.callback;
					( *callback )( change );
				}
			}
			--notificationDepth;
			activeGeneration = parentGeneration;
			if ( notificationDepth == 0 )
				// No active snapshot can reference tombstoned observers now.
				observers.erase( std::remove_if( observers.begin(), observers.end(),
												 []( const Observer& observer ) {
													 return observer.removedGeneration != 0;
												 } ),
								 observers.end() );
		}
		void remove( Uint32 id ) {
			auto it =
				std::find_if( observers.begin(), observers.end(),
							  [id]( const Observer& observer ) { return observer.id == id; } );
			if ( it == observers.end() )
				return;
			if ( notificationDepth != 0 )
				it->removedGeneration = activeGeneration;
			else
				observers.erase( it );
		}
	};

  public:
	/** @brief Move-only scoped ownership of one collection observer. */
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
			if ( auto state = mState.lock() ) {
				auto observer = std::find_if( state->observers.begin(), state->observers.end(),
											  [mId = mId]( const typename State::Observer& item ) {
												  return item.id == mId;
											  } );
				return observer != state->observers.end() && observer->removedGeneration == 0;
			}
			return false;
		}

	  private:
		friend class ObservableVector<T>;
		Connection( const std::shared_ptr<State>& state, Uint32 id ) : mState( state ), mId( id ) {}
		std::weak_ptr<State> mState;
		Uint32 mId{ 0 };
	};

	/**
	 * @brief Shared read and observation access for adapters that may outlive this wrapper.
	 *
	 * Retaining this handle keeps the collection storage alive. Mutations remain available only
	 * through ObservableVector, so the handle becomes a stable read-only snapshot once its owning
	 * wrapper is destroyed.
	 */
	class SharedHandle {
	  public:
		SharedHandle() = default;

		/** @return Read-only access to the retained collection. */
		const std::vector<T>& get() const {
			eeASSERT( mState );
			return mState->values;
		}
		/** @return The retained value at @p index. No bounds checking is performed. */
		const T& operator[]( std::size_t index ) const { return get()[index]; }
		/** @return The number of retained values. */
		std::size_t size() const { return mState ? mState->values.size() : 0; }
		/** @return Whether this handle retains collection storage. */
		explicit operator bool() const { return static_cast<bool>( mState ); }

		/** @return A scoped connection observing later mutations of the owning ObservableVector. */
		Connection observe( Callback callback ) const {
			if ( !mState )
				return {};
			auto id = ++mState->nextId;
			mState->observers.emplace_back( typename State::Observer{
				std::make_shared<Callback>( std::move( callback ) ), 0, id } );
			return Connection( mState, id );
		}

	  private:
		friend class ObservableVector<T>;
		explicit SharedHandle( std::shared_ptr<State> state ) : mState( std::move( state ) ) {}
		std::shared_ptr<State> mState;
	};

	/** @brief Creates an empty observable collection. */
	ObservableVector() : mState( std::make_shared<State>() ) {}

	/** @brief Creates an observable collection containing @p values without emitting a change. */
	explicit ObservableVector( std::vector<T> values ) : mState( std::make_shared<State>() ) {
		mState->values = std::move( values );
	}
	ObservableVector( const ObservableVector& ) = delete;
	ObservableVector& operator=( const ObservableVector& ) = delete;
	ObservableVector( ObservableVector&& ) noexcept = default;
	ObservableVector& operator=( ObservableVector&& ) noexcept = default;

	/** @return Read-only access to the complete collection. */
	const std::vector<T>& get() const { return mState->values; }

	/** @return The value at @p index. No bounds checking is performed. */
	const T& operator[]( std::size_t index ) const { return mState->values[index]; }

	/** @return The number of values in the collection. */
	std::size_t size() const { return mState->values.size(); }

	/** @return Whether the collection contains no values. */
	bool empty() const { return mState->values.empty(); }

	/** @brief Inserts @p value before @p index and emits paired Before/After notifications. */
	void insert( std::size_t index, T value ) {
		eeASSERT( index <= size() );
		notify( ChangeType::Insert, Phase::Before, index, 1 );
		mState->values.insert( mState->values.begin() + index, std::move( value ) );
		notify( ChangeType::Insert, Phase::After, index, 1 );
	}

	/** @brief Appends @p value and emits paired Before/After insertion notifications. */
	void pushBack( T value ) { insert( size(), std::move( value ) ); }

	/** @brief Removes @p count values starting at @p index. A zero count is a no-op. */
	void erase( std::size_t index, std::size_t count = 1 ) {
		eeASSERT( index + count <= size() );
		if ( count == 0 )
			return;
		notify( ChangeType::Remove, Phase::Before, index, count );
		mState->values.erase( mState->values.begin() + index,
							  mState->values.begin() + index + count );
		notify( ChangeType::Remove, Phase::After, index, count );
	}

	/**
	 * @brief Moves one value from @p from to @p to.
	 *
	 * @p to is the final index in the resulting collection. Moving to the same index is a no-op.
	 */
	void move( std::size_t from, std::size_t to ) {
		eeASSERT( from < size() && to < size() );
		if ( from == to )
			return;
		notify( ChangeType::Move, Phase::Before, from, 1, to );
		T value = std::move( mState->values[from] );
		mState->values.erase( mState->values.begin() + from );
		mState->values.insert( mState->values.begin() + to, std::move( value ) );
		notify( ChangeType::Move, Phase::After, from, 1, to );
	}

	/** @brief Replaces the value at @p index unless it already compares equal to @p value. */
	void set( std::size_t index, T value ) {
		eeASSERT( index < size() );
		if ( mState->values[index] == value )
			return;
		notify( ChangeType::Change, Phase::Before, index, 1 );
		mState->values[index] = std::move( value );
		notify( ChangeType::Change, Phase::After, index, 1 );
	}

	/** @brief Replaces the entire collection and emits paired Reset notifications. */
	void reset( std::vector<T> values ) {
		notify( ChangeType::Reset, Phase::Before );
		mState->values = std::move( values );
		notify( ChangeType::Reset, Phase::After );
	}

	/**
	 * @brief Observes subsequent collection mutations.
	 * @return A scoped connection; destroying it disconnects the callback.
	 *
	 * Each non-empty mutation emits a Before notification followed by After. The callback is not
	 * invoked for the collection's current contents when it is registered.
	 */
	Connection observe( Callback callback ) {
		auto id = ++mState->nextId;
		mState->observers.emplace_back( typename State::Observer{
			std::make_shared<Callback>( std::move( callback ) ), 0, id } );
		return Connection( mState, id );
	}

	/** @return Shared read access that keeps collection storage alive independently of this object.
	 */
	SharedHandle sharedHandle() const { return SharedHandle( mState ); }

  private:
	void notify( ChangeType type, Phase phase, std::size_t index = 0, std::size_t count = 0,
				 std::size_t target = 0 ) {
		mState->notify( { type, phase, index, count, target } );
	}
	std::shared_ptr<State> mState;
};

} // namespace EE

#endif
