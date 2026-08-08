#ifndef EE_UI_UIVALUEBINDING_HPP
#define EE_UI_UIVALUEBINDING_HPP

#include <eepp/core/observablevalue.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/uivalueconverter.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

/**
 * @brief Move-only two-way binding between an ObservableValue and a UIWidget property.
 *
 * The binding synchronizes the observable's current value into the widget immediately. Later
 * observable changes update the widget, and the selected widget event converts the property back
 * into the observable. Destroying the binding disconnects both directions. Destroying either the
 * observable or widget first is safe and does not keep that endpoint alive.
 *
 * Synchronization is immediate and single-threaded. The observable, widget, and binding must all be
 * used on the widget's owning UI thread.
 *
 * Use UIValueBinding when an ObservableValue belongs to a UI-independent model. The returned
 * binding must be retained for as long as synchronization is desired.
 *
 * @code
 * ObservableValue<std::string> userName{ "Ada" };
 * auto binding = bindValue( userName, textInput,
 *                      UIValueConverter<std::string>::converterString(),
 *                      "text", Event::OnTextChanged );
 * userName = "Grace"; // Updates textInput without coupling the model to UIWidget.
 * @endcode
 */
template <typename T> class UIValueBinding {
  public:
	using Converter = UIValueConverter<T>;
	static Converter converterDefault() { return Converter::converterDefault(); }

	UIValueBinding() = default;
	UIValueBinding( const UIValueBinding& ) = delete;
	UIValueBinding& operator=( const UIValueBinding& ) = delete;
	UIValueBinding( UIValueBinding&& ) noexcept = default;
	UIValueBinding& operator=( UIValueBinding&& ) noexcept = default;

	UIValueBinding( ObservableValue<T>& value, UIWidget* widget,
					const Converter& converter = converterDefault(),
					const std::string& propertyName = "value",
					Event::EventType eventType = Event::OnValueChange ) {
		connect( value, widget, converter, propertyName, eventType );
	}

	void disconnect() { mState.reset(); }
	explicit operator bool() const { return mState && mState->widget && mState->value; }

  private:
	struct State {
		typename ObservableValue<T>::WeakHandle value;
		UIWidget* widget{ nullptr };
		const PropertyDefinition* property{ nullptr };
		Converter converter;
		bool synchronizing{ false };
		typename ObservableValue<T>::Connection valueConnection;
		EventConnectionList widgetConnections;

		bool applyToWidget( const T& newValue ) {
			if ( !widget )
				return false;
			std::string string;
			if ( !converter.fromValue( property, string, newValue ) ) {
				Log::error( "UIValueBinding: unable to convert observable value to string." );
				return false;
			}
			synchronizing = true;
			widget->applyProperty( StyleSheetProperty( property, string ) );
			synchronizing = false;
			return true;
		}
	};

	void connect( ObservableValue<T>& value, UIWidget* widget, const Converter& converter,
				  const std::string& propertyName, Event::EventType eventType ) {
		eeASSERT( widget != nullptr );
		auto state = std::make_shared<State>();
		state->value = value.weakHandle();
		state->widget = widget;
		state->property = StyleSheetSpecification::instance()->getProperty( propertyName );
		state->converter = converter;
		eeASSERT( state->property != nullptr );
		eeASSERT( state->converter.toValue && state->converter.fromValue );

		std::weak_ptr<State> weakState = state;
		state->valueConnection = value.observe( [weakState]( const T& newValue ) {
			if ( auto state = weakState.lock() )
				state->applyToWidget( newValue );
		} );
		state->widgetConnections += widget->connect( eventType, [weakState]( const Event* event ) {
			if ( auto state = weakState.lock(); state && !state->synchronizing ) {
				T newValue;
				if ( state->converter.toValue(
						 state->property, newValue,
						 event->getNode()->asType<UIWidget>()->getPropertyString(
							 state->property ) ) &&
					 !state->value.set( std::move( newValue ) ) ) {
					state->widget = nullptr;
					state->widgetConnections.clear();
				}
			}
		} );
		state->widgetConnections += widget->connect( Event::OnClose, [weakState]( const Event* ) {
			if ( auto state = weakState.lock() ) {
				state->widget = nullptr;
				state->valueConnection.disconnect();
				state->widgetConnections.clear();
			}
		} );
		state->applyToWidget( value.get() );
		mState = std::move( state );
	}

	std::shared_ptr<State> mState;
};

/** @brief Creates a scoped two-way binding between @p value and @p widget. */
template <typename T>
UIValueBinding<T> bindValue(
	ObservableValue<T>& value, UIWidget* widget,
	const typename UIValueBinding<T>::Converter& converter = UIValueBinding<T>::converterDefault(),
	const std::string& propertyName = "value", Event::EventType eventType = Event::OnValueChange ) {
	return UIValueBinding<T>( value, widget, converter, propertyName, eventType );
}

}} // namespace EE::UI

#endif
