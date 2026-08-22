#ifndef EE_UI_UIPROPERTY_HPP
#define EE_UI_UIPROPERTY_HPP

#include <eepp/ui/databinding/uidatabind.hpp>
#include <type_traits>

namespace EE { namespace UI {

/**
 * @brief Owns a value and exposes it as a UIDataBind-backed widget property.
 *
 * UIProperty is the owning counterpart to UIDataBind: the synchronized value is stored inside the
 * property, so callers only need to ensure the UIProperty itself remains alive while using it.
 * Assignments propagate to connected widgets, and widget-originated changes update value().
 * Connections are removed automatically when either the UIProperty or a connected widget dies.
 *
 * The class is non-copyable and non-movable because its UIDataBind stores the address of mValue and
 * installs callbacks that capture the binding's address.
 *
 * Use UIProperty for concise UI-local state when the value and its widgets naturally share a
 * lifetime. It avoids declaring a separate model value and binding, and owns its UIDataBind
 * directly. A custom UIValueConverter can provide presentation-specific parsing and formatting.
 *
 * UIProperty also implements the common observable-source interface (ValueType, get(), and
 * observe()), so it can directly feed ComputedValue and UICommand. UIBindingGroup can track a
 * property with `form += property`, including its validation, dirty state, and connected widgets.
 *
 * @code
 * UIProperty<double> celsius( 0.0, celsiusInput );
 * UIProperty<double> fahrenheit( 32.0, fahrenheitInput );
 * celsius.changed( [&fahrenheit]( double value ) {
 *     fahrenheit = value * 9.0 / 5.0 + 32.0;
 * } );
 * @endcode
 */
template <typename T> class UIProperty {
  private:
	struct LifetimeState {
		LifetimeState( UIProperty<T>* property, T value ) :
			value( std::move( value ) ), property( property ) {}
		T value;
		UIProperty<T>* property{ nullptr };
		// Most UIProperty instances are not held by UIBindingGroup. Allocate the lifetime
		// observable only when a consumer actually subscribes to destruction.
		std::unique_ptr<ObservableValue<bool>> alive;
	};

  public:
	using ValueType = T;
	using Callback = typename UIDataBind<T>::Callback;
	using Connection = typename UIDataBind<T>::Connection;
	using ValidationConnection = typename UIValueValidationState::Connection;

	/**
	 * @brief Lifetime-safe, non-owning access used by containers such as UIBindingGroup.
	 *
	 * Every operation becomes a harmless no-op or empty result after the UIProperty is destroyed.
	 * Like UIProperty itself, this handle is restricted to the widgets' owning UI thread.
	 */
	class WeakHandle {
	  public:
		WeakHandle() = default;

		/** @return A copy of the value, or std::nullopt after expiration or binding reset. */
		std::optional<T> get() const {
			auto state = mState.lock();
			return state && state->property && state->property->databind().isInitialized()
					   ? std::optional<T>( state->property->get() )
					   : std::nullopt;
		}

		/** @return true when the live property was assigned @p value. */
		bool set( const T& value ) const {
			auto state = mState.lock();
			if ( !state || !state->property || !state->property->databind().isInitialized() )
				return false;
			*state->property = value;
			return true;
		}

		/** @return A scoped value observer connection, or an empty connection after expiration. */
		Connection observe( Callback callback ) const {
			auto state = mState.lock();
			return state && state->property && state->property->databind().isInitialized()
					   ? state->property->observe( std::move( callback ) )
					   : Connection{};
		}

		/** @return A scoped connection notified when the property is about to expire. */
		ObservableValue<bool>::Connection
		observeLifetime( ObservableValue<bool>::Callback callback ) const {
			auto state = mState.lock();
			if ( !state )
				return {};
			if ( !state->alive )
				state->alive = std::make_unique<ObservableValue<bool>>( true );
			return state->alive->observe( std::move( callback ) );
		}

		/** @return A scoped validation observer, or an empty connection after expiration. */
		ValidationConnection observeValidation( UIValueValidationState::Callback callback ) const {
			auto state = mState.lock();
			return state && state->property && state->property->databind().isInitialized()
					   ? state->property->databind().validationState().observe(
							 std::move( callback ) )
					   : ValidationConnection{};
		}

		/** @return Current validation, or success after expiration. */
		UIValueValidationResult validation() const {
			auto state = mState.lock();
			return state && state->property && state->property->databind().isInitialized()
					   ? state->property->validationState().result()
					   : UIValueValidationResult::success();
		}

		/** @return The widget that produced the current input error, or nullptr. */
		UIWidget* validationEmitter() const {
			auto state = mState.lock();
			return state && state->property && state->property->databind().isInitialized()
					   ? state->property->databind().getValidationEmitter()
					   : nullptr;
		}

		/** @return All currently connected widgets, or an empty vector after expiration. */
		std::vector<UIWidget*> widgets() const {
			auto state = mState.lock();
			if ( !state || !state->property || !state->property->databind().isInitialized() )
				return {};
			const auto& widgets = state->property->databind().getWidgets();
			return { widgets.begin(), widgets.end() };
		}

		/** Appends connected widgets without creating an intermediate collection. */
		template <typename Container> void appendWidgets( Container& destination ) const {
			auto state = mState.lock();
			if ( !state || !state->property || !state->property->databind().isInitialized() )
				return;
			const auto& widgets = state->property->databind().getWidgets();
			destination.insert( destination.end(), widgets.begin(), widgets.end() );
		}

		/** Invokes @p callback for every connected widget without allocating a collection. */
		template <typename WidgetCallback> void forEachWidget( WidgetCallback&& callback ) const {
			auto state = mState.lock();
			if ( !state || !state->property || !state->property->databind().isInitialized() )
				return;
			for ( auto widget : state->property->databind().getWidgets() )
				callback( widget );
		}

		/** @return The first enabled connected widget, or nullptr. */
		UIWidget* firstEnabledWidget() const {
			auto state = mState.lock();
			if ( !state || !state->property || !state->property->databind().isInitialized() )
				return nullptr;
			for ( auto widget : state->property->databind().getWidgets() )
				if ( widget && widget->isEnabled() )
					return widget;
			return nullptr;
		}

		explicit operator bool() const {
			auto state = mState.lock();
			return state && state->property && state->property->databind().isInitialized();
		}

	  private:
		friend class UIProperty<T>;
		explicit WeakHandle( const std::shared_ptr<LifetimeState>& state ) : mState( state ) {}
		std::weak_ptr<LifetimeState> mState;
	};

	UIProperty( const UIProperty& ) = delete;
	UIProperty& operator=( const UIProperty& ) = delete;
	UIProperty( UIProperty&& ) = delete;
	UIProperty& operator=( UIProperty&& ) = delete;
	~UIProperty() {
		mLifetime->property = nullptr;
		if ( mLifetime->alive )
			*mLifetime->alive = false;
	}

	UIProperty( T defaultValue, UIWidget* widget,
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mLifetime( std::make_shared<LifetimeState>( this, std::move( defaultValue ) ) ) {
		initializeBinding( widget, converter, valueKey, eventType );
	}

	UIProperty( T defaultValue, const UnorderedSet<UIWidget*>& widgets = {},
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mLifetime( std::make_shared<LifetimeState>( this, std::move( defaultValue ) ) ) {
		initializeBinding( widgets, converter, valueKey, eventType );
	}

	UIProperty( const UnorderedSet<UIWidget*>& widgets = {},
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mLifetime( std::make_shared<LifetimeState>( this, T{} ) ) {
		initializeBinding( widgets, converter, valueKey, eventType );
	}

	UIProperty( UIWidget* widget,
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mLifetime( std::make_shared<LifetimeState>( this, T{} ) ) {
		initializeBinding( widget, converter, valueKey, eventType );
	}

	UIProperty& operator=( const T& newVal ) {
		mBindedData.set( newVal );
		return *this;
	}

	UIProperty& operator=( T&& newVal ) noexcept {
		mBindedData.set( std::move( newVal ) );
		return *this;
	}

	/** @name Value mutation
	 * Compound assignment propagates through the binding like assignment. Arithmetic properties
	 * support the conventional numeric mutations; std::string and String properties support
	 * concatenation.
	 * @{ */
	template <typename U = T,
			  std::enable_if_t<(std::is_arithmetic_v<U> && !std::is_same_v<U, bool>) ||
								   std::is_same_v<U, std::string> || std::is_same_v<U, String>,
							   int> = 0>
	UIProperty& operator+=( const T& operand ) {
		return *this = value() + operand;
	}

	template <
		typename U = T,
		std::enable_if_t<std::is_same_v<U, std::string> || std::is_same_v<U, String>, int> = 0>
	T operator+( const T& operand ) const {
		return value() + operand;
	}

	template <typename U = T,
			  std::enable_if_t<std::is_arithmetic_v<U> && !std::is_same_v<U, bool>, int> = 0>
	UIProperty& operator-=( const T& operand ) {
		return *this = value() - operand;
	}

	template <typename U = T,
			  std::enable_if_t<std::is_arithmetic_v<U> && !std::is_same_v<U, bool>, int> = 0>
	UIProperty& operator*=( const T& operand ) {
		return *this = value() * operand;
	}

	template <typename U = T,
			  std::enable_if_t<std::is_arithmetic_v<U> && !std::is_same_v<U, bool>, int> = 0>
	UIProperty& operator/=( const T& operand ) {
		return *this = value() / operand;
	}

	template <typename U = T,
			  std::enable_if_t<std::is_arithmetic_v<U> && !std::is_same_v<U, bool>, int> = 0>
	UIProperty& operator++() {
		return *this += 1;
	}

	template <typename U = T,
			  std::enable_if_t<std::is_arithmetic_v<U> && !std::is_same_v<U, bool>, int> = 0>
	T operator++( int ) {
		T previous = value();
		++( *this );
		return previous;
	}

	template <typename U = T,
			  std::enable_if_t<std::is_arithmetic_v<U> && !std::is_same_v<U, bool>, int> = 0>
	UIProperty& operator--() {
		return *this -= 1;
	}

	template <typename U = T,
			  std::enable_if_t<std::is_arithmetic_v<U> && !std::is_same_v<U, bool>, int> = 0>
	T operator--( int ) {
		T previous = value();
		--( *this );
		return previous;
	}
	/** @} */

	/** @return The current synchronized value. */
	const T& value() const { return mBindedData.get(); }

	/** @return The current synchronized value; enables the common observable-source interface. */
	const T& get() const { return value(); }

	/**
	 * @brief Observes later model- or widget-originated value changes.
	 * @return A scoped connection; destroying it disconnects the callback.
	 */
	Connection observe( Callback callback ) { return mBindedData.observe( std::move( callback ) ); }

	/** @return A non-owning handle that expires safely when this property is destroyed. */
	WeakHandle weakHandle() { return WeakHandle( mLifetime ); }

	const UIDataBind<T>& databind() const { return mBindedData; }

	UIDataBind<T>& databind() { return mBindedData; }

	/** @return Current converter error state. */
	const UIValueValidationState& validationState() const { return mBindedData.validationState(); }

	/** @brief Connects another widget to this property's value. */
	UIProperty& connect( UIWidget* widget ) {
		mBindedData.bind( widget );
		return *this;
	}

	/** @brief Disconnects a widget from this property's value. */
	UIProperty& disconnect( UIWidget* widget ) {
		mBindedData.unbind( widget );
		return *this;
	}

	const T& operator*() const noexcept { return value(); }

	const T* operator->() const noexcept { return &value(); }

	operator const T&() const noexcept { return value(); }

	/** @brief Sets the callback invoked after the synchronized value changes. */
	UIProperty& changed( const std::function<void( const T& newVal )>& fn ) {
		mBindedData.onValueChangeCb = fn;
		return *this;
	}

	UIProperty& changed( std::function<void( const T& newVal )>&& fn ) {
		mBindedData.onValueChangeCb = std::move( fn );
		return *this;
	}

  protected:
	template <typename Widgets>
	void initializeBinding( Widgets&& widgets,
							const typename EE::UI::UIDataBind<T>::Converter& converter,
							const std::string& valueKey, const Event::EventType& eventType ) {
		// Retain the complete lifetime state while accessing its embedded value directly. Keeping
		// ownership separate avoids both an extra value allocation and an aliasing shared_ptr.
		mBindedData.initRetained( &mLifetime->value, mLifetime, std::forward<Widgets>( widgets ),
								  converter, valueKey, eventType );
	}

	// Declaration order is intentional: the binding aliases mLifetime's allocation and must be
	// destroyed before the final owning reference is released.
	std::shared_ptr<LifetimeState> mLifetime;
	UIDataBind<T> mBindedData;
};

}} // namespace EE::UI

#endif
