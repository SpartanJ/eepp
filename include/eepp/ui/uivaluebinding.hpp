#ifndef EE_UI_UIVALUEBINDING_HPP
#define EE_UI_UIVALUEBINDING_HPP

#include <eepp/core/observablevalue.hpp>
#include <eepp/ui/uivalueconverter.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

/**
 * @brief Move-only two-way binding between an ObservableValue and a UIWidget property.
 *
 * The converter maps directly between T and the widget property string. Its toValue() callback
 * decides whether widget input may enter the model. Model-originated values are authoritative and
 * are formatted through fromValue().
 *
 * Destroying the binding disconnects both directions. Destroying either the observable or widget
 * first is safe and does not keep that endpoint alive.
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
	bool isValid() const { return !mState || mState->validation.isValid(); }

	/** @return Observable conversion and input-validation state. */
	UIValueValidationState* validationState() { return mState ? &mState->validation : nullptr; }
	const UIValueValidationState* validationState() const {
		return mState ? &mState->validation : nullptr;
	}

  private:
	struct State {
		typename ObservableValue<T>::WeakHandle value;
		UIWidget* widget{ nullptr };
		const PropertyDefinition* property{ nullptr };
		Converter converter;
		UIValueValidationState validation;
		bool synchronizing{ false };
		typename ObservableValue<T>::Connection valueConnection;
		EventConnectionList widgetConnections;

		bool applyToWidget( const T& newValue ) {
			if ( !widget )
				return false;
			auto converted = converter.fromValue( property, newValue );
			if ( !converted ) {
				validation.set( std::move( converted.validation ) );
				return false;
			}
			synchronizing = true;
			widget->applyProperty( StyleSheetProperty( property, *converted.value ) );
			synchronizing = false;
			validation.clear();
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
				auto proposed = state->converter.toValue(
					state->property,
					event->getNode()->asType<UIWidget>()->getPropertyString( state->property ) );
				if ( !proposed ) {
					state->validation.set( std::move( proposed.validation ) );
					return;
				}
				state->validation.clear();
				if ( !state->value.set( std::move( *proposed.value ) ) ) {
					state->widget = nullptr;
					state->widgetConnections.clear();
				}
			}
		} );
		state->widgetConnections += widget->connect( Event::OnClose, [weakState]( const Event* ) {
			if ( auto state = weakState.lock() ) {
				state->widget = nullptr;
				state->valueConnection.disconnect();
				state->validation.clear();
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
UIValueBinding<T>
bindValue( ObservableValue<T>& value, UIWidget* widget,
		   const UIValueConverter<T>& converter = UIValueConverter<T>::converterDefault(),
		   const std::string& propertyName = "value",
		   Event::EventType eventType = Event::OnValueChange ) {
	return UIValueBinding<T>( value, widget, converter, propertyName, eventType );
}

}} // namespace EE::UI

#endif
