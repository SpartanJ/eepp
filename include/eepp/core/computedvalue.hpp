#ifndef EE_CORE_COMPUTEDVALUE_HPP
#define EE_CORE_COMPUTEDVALUE_HPP

#include <eepp/core/observablevalue.hpp>
#include <tuple>
#include <type_traits>
#include <utility>

namespace EE {

/**
 * @brief A read-only observable whose value is synchronously derived from explicit dependencies.
 *
 * Dependencies are observed in argument order. A dependency change updates its cached value and
 * recomputes the result before the dependency's set() returns. Destroying a dependency leaves the
 * computed value at its last result; destroying either endpoint safely expires scoped observers.
 * ComputedValue is single-threaded, like ObservableValue.
 */
template <typename T, typename Calculator, typename... Dependencies> class ComputedValue {
  private:
	using Values = std::tuple<typename Dependencies::ValueType...>;
	using Connections = std::tuple<typename Dependencies::Connection...>;

	struct State {
		State( Calculator calculator, Values values ) :
			calculator( std::move( calculator ) ),
			values( std::move( values ) ),
			output( calculate() ) {}

		T calculate() {
			return std::apply( [this]( const auto&... value ) { return calculator( value... ); },
							   values );
		}

		void recompute() { output.set( calculate() ); }

		Calculator calculator;
		Values values;
		ObservableValue<T> output;
	};

	template <std::size_t... Is>
	ComputedValue( Calculator calculator, std::index_sequence<Is...>,
				   Dependencies&... dependencies ) :
		mState(
			std::make_shared<State>( std::move( calculator ), Values( dependencies.get()... ) ) ) {
		// The comma fold guarantees dependency registration follows argument order.
		( ( std::get<Is>( mConnections ) = connect<Is>( dependencies ) ), ... );
	}

	template <std::size_t I, typename Dependency>
	typename Dependency::Connection connect( Dependency& dependency ) {
		std::weak_ptr<State> weakState = mState;
		return dependency.observe( [weakState]( const typename Dependency::ValueType& value ) {
			if ( auto state = weakState.lock() ) {
				std::get<I>( state->values ) = value;
				state->recompute();
			}
		} );
	}

  public:
	using ValueType = T;
	using Callback = typename ObservableValue<T>::Callback;
	using Connection = typename ObservableValue<T>::Connection;

	/**
	 * @brief Creates a computed value and evaluates @p calculator once from the current
	 * dependencies.
	 *
	 * The calculator receives the dependency values as const references in the same order in which
	 * the dependencies are passed. Every dependency must outlive this object if further updates are
	 * expected from it.
	 */
	ComputedValue( Calculator calculator, Dependencies&... dependencies ) :
		ComputedValue( std::move( calculator ), std::index_sequence_for<Dependencies...>{},
					   dependencies... ) {}
	ComputedValue( const ComputedValue& ) = delete;
	ComputedValue& operator=( const ComputedValue& ) = delete;
	ComputedValue( ComputedValue&& ) noexcept = default;
	ComputedValue& operator=( ComputedValue&& ) noexcept = default;

	/** @return The most recently calculated value. */
	const T& get() const { return mState->output.get(); }

	const T& operator*() const { return get(); }

	const T* operator->() const { return &get(); }

	operator const T&() const { return get(); }

	/**
	 * @brief Observes later changes to the calculated value.
	 * @return A scoped connection; destroying it disconnects the callback.
	 *
	 * The callback is not invoked immediately. Read get() when the initial value is needed.
	 */
	Connection observe( Callback callback ) {
		return mState->output.observe( std::move( callback ) );
	}

	/** @return The number of observers currently attached to the calculated output. */
	std::size_t observerCount() const { return mState->output.observerCount(); }

	/** @return The number of observable dependencies captured by this computed value. */
	static constexpr std::size_t dependencyCount() { return sizeof...( Dependencies ); }

  private:
	std::shared_ptr<State> mState;
	Connections mConnections;
};

template <typename Calculator, typename... Dependencies>
/** @brief Creates a ComputedValue while deducing its result, calculator, and dependency types. */
auto makeComputedValue( Calculator&& calculator, Dependencies&... dependencies ) {
	using StoredCalculator = std::decay_t<Calculator>;
	using Result = std::decay_t<
		std::invoke_result_t<StoredCalculator, const typename Dependencies::ValueType&...>>;
	return ComputedValue<Result, StoredCalculator, Dependencies...>(
		std::forward<Calculator>( calculator ), dependencies... );
}

template <typename Dependency, typename Calculator>
/** @brief Convenience form of makeComputedValue() for one dependency. */
auto computedValue( Dependency& dependency, Calculator&& calculator ) {
	return makeComputedValue( std::forward<Calculator>( calculator ), dependency );
}

template <typename Dependency1, typename Dependency2, typename Calculator>
/** @brief Convenience form of makeComputedValue() for two dependencies. */
auto computedValue( Dependency1& dependency1, Dependency2& dependency2, Calculator&& calculator ) {
	return makeComputedValue( std::forward<Calculator>( calculator ), dependency1, dependency2 );
}

template <typename Dependency1, typename Dependency2, typename Dependency3, typename Calculator>
/** @brief Convenience form of makeComputedValue() for three dependencies. */
auto computedValue( Dependency1& dependency1, Dependency2& dependency2, Dependency3& dependency3,
					Calculator&& calculator ) {
	return makeComputedValue( std::forward<Calculator>( calculator ), dependency1, dependency2,
							  dependency3 );
}

template <typename Dependency1, typename Dependency2, typename Dependency3, typename Dependency4,
		  typename Calculator>
/** @brief Convenience form of makeComputedValue() for four dependencies. */
auto computedValue( Dependency1& dependency1, Dependency2& dependency2, Dependency3& dependency3,
					Dependency4& dependency4, Calculator&& calculator ) {
	return makeComputedValue( std::forward<Calculator>( calculator ), dependency1, dependency2,
							  dependency3, dependency4 );
}

} // namespace EE

#endif
