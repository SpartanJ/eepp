#include <eepp/ui/uidatabind.hpp>
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
 * lifetime. It avoids the shared state required by ObservableValue and owns its UIDataBind
 * directly.
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
  public:
	UIProperty( const UIProperty& ) = delete;
	UIProperty& operator=( const UIProperty& ) = delete;
	UIProperty( UIProperty&& ) = delete;
	UIProperty& operator=( UIProperty&& ) = delete;

	UIProperty( T defaultValue, UIWidget* widget,
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mValue( std::move( defaultValue ) ),
		mBindedData( &mValue, widget, converter, valueKey, eventType ) {}

	UIProperty( T defaultValue, const UnorderedSet<UIWidget*>& widgets = {},
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mValue( std::move( defaultValue ) ),
		mBindedData( &mValue, widgets, converter, valueKey, eventType ) {}

	UIProperty( const UnorderedSet<UIWidget*>& widgets = {},
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mBindedData( &mValue, widgets, converter, valueKey, eventType ) {}

	UIProperty( UIWidget* widget,
				const typename EE::UI::UIDataBind<T>::Converter& converter =
					EE::UI::UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) :
		mBindedData( &mValue, widget, converter, valueKey, eventType ) {}

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

	const T& value() const { return mBindedData.get(); }

	const UIDataBind<T>& databind() const { return mBindedData; }

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
	T mValue{};
	UIDataBind<T> mBindedData;
};

}} // namespace EE::UI
