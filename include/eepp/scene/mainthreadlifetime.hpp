#ifndef EE_SCENE_MAINTHREADLIFETIME_HPP
#define EE_SCENE_MAINTHREADLIFETIME_HPP

#include <eepp/scene/node.hpp>
#include <functional>
#include <memory>
#include <mutex>

namespace EE::Scene {

/**
 * Gates main-thread callbacks with the lifetime of a non-owning object.
 *
 * The dispatcher and the guarded object must belong to the main thread. The dispatcher may be the
 * guarded object itself, but it must otherwise outlive it. Worker threads may safely copy a
 * WeakHandle and call run(); callbacks queued before invalidation become no-ops once the object is
 * destroyed.
 */
template <typename T> class MainThreadLifetime {
  private:
	struct State {
		State( T* object, Node* dispatcher ) : object( object ), dispatcher( dispatcher ) {}
		std::mutex mutex;
		T* object{ nullptr };
		Node* dispatcher{ nullptr };
	};

  public:
	class WeakHandle {
	  public:
		WeakHandle() = default;

		void run( std::function<void( T* )> callback, const Time& delay = Time::Zero,
				  Action::UniqueID tag = 0 ) const {
			auto state = mState.lock();
			if ( !state )
				return;
			std::lock_guard<std::mutex> lock( state->mutex );
			if ( !state->object || !state->dispatcher )
				return;
			state->dispatcher->runOnMainThread(
				[state = std::weak_ptr<State>{ state }, callback = std::move( callback )] {
					auto lockedState = state.lock();
					if ( !lockedState )
						return;
					T* object;
					{
						std::lock_guard<std::mutex> lock( lockedState->mutex );
						object = lockedState->object;
					}
					if ( object )
						callback( object );
				},
				delay, tag );
		}

		explicit operator bool() const { return !mState.expired(); }

	  private:
		friend class MainThreadLifetime<T>;
		explicit WeakHandle( const std::shared_ptr<State>& state ) : mState( state ) {}
		std::weak_ptr<State> mState;
	};

	MainThreadLifetime( T* object, Node* dispatcher ) :
		mState( std::make_shared<State>( object, dispatcher ) ) {}

	~MainThreadLifetime() { invalidate(); }

	MainThreadLifetime( const MainThreadLifetime& ) = delete;
	MainThreadLifetime& operator=( const MainThreadLifetime& ) = delete;

	WeakHandle weakHandle() const { return WeakHandle{ mState }; }

	void setDispatcher( Node* dispatcher ) {
		std::lock_guard<std::mutex> lock( mState->mutex );
		if ( mState->object )
			mState->dispatcher = dispatcher;
	}

	void invalidate() {
		std::lock_guard<std::mutex> lock( mState->mutex );
		mState->object = nullptr;
		mState->dispatcher = nullptr;
	}

  private:
	std::shared_ptr<State> mState;
};

} // namespace EE::Scene

#endif
