#ifndef EE_UI_UIOBSERVEDELIVERY_HPP
#define EE_UI_UIOBSERVEDELIVERY_HPP

#include <eepp/core/observablevalue.hpp>
#include <eepp/scene/node.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <mutex>

namespace EE { namespace UI {

/**
 * @brief Scoped non-blocking delivery of observable changes to the UI thread.
 *
 * The scheduler must outlive this connection and is normally the owning UISceneNode. The endpoint
 * widget is not retained; queued work becomes a no-op after it closes. Source mutation remains the
 * producer's synchronization responsibility because ObservableValue itself is single-threaded.
 * In particular, construct and disconnect this observation only while the producer is stopped or
 * otherwise synchronized; observer registration and removal must not race source mutation.
 */
template <typename T> class UIThreadObservation {
  public:
	using Callback = std::function<void( UIWidget&, const T& )>;

	UIThreadObservation() = default;
	UIThreadObservation( const UIThreadObservation& ) = delete;
	UIThreadObservation& operator=( const UIThreadObservation& ) = delete;
	UIThreadObservation( UIThreadObservation&& ) noexcept = default;
	UIThreadObservation& operator=( UIThreadObservation&& ) noexcept = default;

	/**
	 * @brief Observes @p source and queues @p callback on @p scheduler's UI thread.
	 *
	 * Delivery preserves source notification order. The callback receives the endpoint only while
	 * it remains alive. Keep the returned observation alive and ensure the scheduler outlives it.
	 */
	template <typename Source>
	UIThreadObservation( Source& source, Node& scheduler, UIWidget& endpoint, Callback callback ) {
		auto state = std::make_shared<State>();
		state->endpoint = &endpoint;
		state->callback = std::move( callback );
		std::weak_ptr<State> weakState = state;
		state->endpointConnection = endpoint.connect( Event::OnClose, [weakState]( const Event* ) {
			if ( auto state = weakState.lock() ) {
				std::lock_guard<std::mutex> lock( state->mutex );
				state->endpoint = nullptr;
			}
		} );
		auto sourceConnection =
			source.observe( [weakState, scheduler = &scheduler]( const T& value ) {
				if ( auto state = weakState.lock() ) {
					std::lock_guard<std::mutex> lock( state->mutex );
					if ( !state->endpoint )
						return;
				} else {
					return;
				}
				// Runnable uses SmallFunction<48>, so common small values travel inline with no
				// per-delivery allocation. Large values use the scheduler's existing heap fallback.
				scheduler->ensureMainThread( [weakState, delivery = T( value )] {
					if ( auto state = weakState.lock() ) {
						UIWidget* endpoint = nullptr;
						{
							std::lock_guard<std::mutex> lock( state->mutex );
							endpoint = state->endpoint;
						}
						// Delivery runs on the UI thread, so the endpoint cannot close between this
						// check and the callback except from within the callback itself.
						if ( endpoint )
							state->callback( *endpoint, delivery );
					}
				} );
			} );
		state->sourceConnection = std::make_unique<SourceConnection<decltype( sourceConnection )>>(
			std::move( sourceConnection ) );
		mState = std::move( state );
	}

	/**
	 * @brief Stops future delivery and invalidates already queued callbacks.
	 *
	 * Synchronize with the source producer before calling this; see the class thread-safety notes.
	 */
	void disconnect() {
		if ( mState ) {
			mState->sourceConnection->disconnect();
			std::lock_guard<std::mutex> lock( mState->mutex );
			mState->endpoint = nullptr;
		}
		mState.reset();
	}

	/** @return Whether the source is connected and the endpoint remains alive. */
	explicit operator bool() const {
		if ( !mState || !mState->sourceConnection || !mState->sourceConnection->connected() )
			return false;
		std::lock_guard<std::mutex> lock( mState->mutex );
		return mState->endpoint != nullptr;
	}

  private:
	struct SourceConnectionBase {
		virtual ~SourceConnectionBase() = default;
		virtual void disconnect() = 0;
		virtual bool connected() const = 0;
	};

	template <typename Connection> struct SourceConnection final : SourceConnectionBase {
		explicit SourceConnection( Connection connection ) :
			connection( std::move( connection ) ) {}
		void disconnect() override { connection.disconnect(); }
		bool connected() const override { return static_cast<bool>( connection ); }
		Connection connection;
	};

	struct State {
		mutable std::mutex mutex;
		UIWidget* endpoint{ nullptr };
		Callback callback;
		EventConnection endpointConnection;
		// Source types expose different scoped connection classes. Type erasure happens once when
		// constructing the observation and adds no work or allocation to value delivery.
		std::unique_ptr<SourceConnectionBase> sourceConnection;
	};
	std::shared_ptr<State> mState;
};

template <typename Source, typename Callback>
/**
 * @brief Creates a scoped UI-thread observation while deducing the source value type.
 * @see UIThreadObservation
 */
auto observeOnUIThread( Source& source, Node& scheduler, UIWidget& endpoint, Callback&& callback ) {
	using T = typename Source::ValueType;
	return UIThreadObservation<T>( source, scheduler, endpoint,
								   std::forward<Callback>( callback ) );
}

}} // namespace EE::UI

#endif
