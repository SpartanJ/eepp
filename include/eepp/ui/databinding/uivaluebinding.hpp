#ifndef EE_UI_UIVALUEBINDING_HPP
#define EE_UI_UIVALUEBINDING_HPP

#include <eepp/core/computedvalue.hpp>
#include <eepp/core/observablevalue.hpp>
#include <eepp/ui/databinding/uivalueconverter.hpp>
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

	/** @return The standard converter for the bound value type. */
	static Converter converterDefault() { return Converter::converterDefault(); }

	UIValueBinding() = default;
	UIValueBinding( const UIValueBinding& ) = delete;
	UIValueBinding& operator=( const UIValueBinding& ) = delete;
	UIValueBinding( UIValueBinding&& ) noexcept = default;
	UIValueBinding& operator=( UIValueBinding&& ) noexcept = default;

	/**
	 * @brief Starts synchronizing @p value with a property of @p widget.
	 *
	 * The current model value is applied to the widget immediately. Later @p eventType events parse
	 * the widget property back into the model.
	 */
	UIValueBinding( ObservableValue<T>& value, UIWidget* widget,
					const Converter& converter = converterDefault(),
					const std::string& propertyName = "value",
					Event::EventType eventType = Event::OnValueChange ) {
		connect( value, widget, converter, propertyName, eventType );
	}

	/** @brief Stops synchronization in both directions. Calling this repeatedly is safe. */
	void disconnect() { mState.reset(); }

	/** @return Whether both model and widget endpoints are still alive and connected. */
	explicit operator bool() const { return mState && mState->widget && mState->value; }

	/** @return Whether both model and widget endpoints are still alive and connected. */
	bool isConnected() const { return static_cast<bool>( *this ); }

	/** @return Whether the most recent conversion or validation succeeded. */
	bool isValid() const { return !mState || mState->validation.isValid(); }

	/** @return The bound widget, or nullptr after disconnection or widget destruction. */
	UIWidget* widget() const { return mState ? mState->widget : nullptr; }

	/** @return A copy of the model value, or std::nullopt after disconnection. */
	std::optional<T> value() const { return mState ? mState->value.get() : std::nullopt; }

	/** @return true when the connected model still exists and was assigned @p value. */
	bool setValue( const T& value ) { return mState && mState->value.set( value ); }

	/** @return A scoped observer connection to later model changes, or an empty connection. */
	typename ObservableValue<T>::Connection
	observeValue( typename ObservableValue<T>::Callback callback ) {
		return mState ? mState->value.observe( std::move( callback ) )
					  : typename ObservableValue<T>::Connection{};
	}

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

/**
 * @brief Move-only one-way binding from a read-only observable to a widget property.
 *
 * Sources must provide ValueType, get(), observe(), and an ObservableValue-compatible Connection.
 * The binding applies the current source value immediately and retains neither endpoint. Use this
 * for ComputedValue outputs or whenever widget edits must not update the source.
 */
template <typename T> class UIReadOnlyValueBinding {
  public:
	using Converter = UIValueConverter<T>;

	UIReadOnlyValueBinding() = default;
	UIReadOnlyValueBinding( const UIReadOnlyValueBinding& ) = delete;
	UIReadOnlyValueBinding& operator=( const UIReadOnlyValueBinding& ) = delete;
	UIReadOnlyValueBinding( UIReadOnlyValueBinding&& ) noexcept = default;
	UIReadOnlyValueBinding& operator=( UIReadOnlyValueBinding&& ) noexcept = default;

	/** @brief Starts one-way synchronization from @p source to a property of @p widget. */
	template <typename Source>
	UIReadOnlyValueBinding( Source& source, UIWidget* widget,
							const Converter& converter = Converter::converterDefault(),
							const std::string& propertyName = "value" ) {
		connect( source, widget, converter, propertyName );
	}

	/** @brief Stops synchronization. Calling this repeatedly is safe. */
	void disconnect() { mState.reset(); }

	/** @return Whether the source connection and widget endpoint remain active. */
	explicit operator bool() const {
		return mState && mState->widget && static_cast<bool>( mState->sourceConnection );
	}

	/** @return Whether formatting the most recent source value succeeded. */
	bool isValid() const { return !mState || mState->validation.isValid(); }

	/** @return Formatting validation state, or nullptr for an empty binding. */
	const UIValueValidationState* validationState() const {
		return mState ? &mState->validation : nullptr;
	}

  private:
	struct State {
		UIWidget* widget{ nullptr };
		const PropertyDefinition* property{ nullptr };
		Converter converter;
		UIValueValidationState validation;
		typename ObservableValue<T>::Connection sourceConnection;
		EventConnection widgetConnection;

		void applyToWidget( const T& value ) {
			if ( !widget )
				return;
			auto converted = converter.fromValue( property, value );
			if ( !converted ) {
				validation.set( std::move( converted.validation ) );
				return;
			}
			widget->applyProperty( StyleSheetProperty( property, *converted.value ) );
			validation.clear();
		}
	};

	template <typename Source>
	void connect( Source& source, UIWidget* widget, const Converter& converter,
				  const std::string& propertyName ) {
		eeASSERT( widget != nullptr );
		auto state = std::make_shared<State>();
		state->widget = widget;
		state->property = StyleSheetSpecification::instance()->getProperty( propertyName );
		state->converter = converter;
		eeASSERT( state->property != nullptr );
		eeASSERT( state->converter.fromValue );

		std::weak_ptr<State> weakState = state;
		state->sourceConnection = source.observe( [weakState]( const T& value ) {
			if ( auto state = weakState.lock() )
				state->applyToWidget( value );
		} );
		state->widgetConnection = widget->connect( Event::OnClose, [weakState]( const Event* ) {
			if ( auto state = weakState.lock() ) {
				state->widget = nullptr;
				state->sourceConnection.disconnect();
				state->validation.clear();
			}
		} );
		state->applyToWidget( source.get() );
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

/** @brief Creates a two-way binding to a non-default widget property using default conversion. */
template <typename T>
UIValueBinding<T> bindValue( ObservableValue<T>& value, UIWidget* widget,
							 const std::string& propertyName,
							 Event::EventType eventType = Event::OnValueChange ) {
	return UIValueBinding<T>( value, widget, UIValueConverter<T>::converterDefault(), propertyName,
							  eventType );
}

/** @brief Creates a scoped one-way binding from a computed value to a widget. */
template <typename T, typename Calculator, typename... Dependencies>
UIReadOnlyValueBinding<T>
bindValue( ComputedValue<T, Calculator, Dependencies...>& value, UIWidget* widget,
		   const UIValueConverter<T>& converter = UIValueConverter<T>::converterDefault(),
		   const std::string& propertyName = "value" ) {
	return UIReadOnlyValueBinding<T>( value, widget, converter, propertyName );
}

/** @brief Creates a read-only binding to a non-default property using default conversion. */
template <typename T, typename Calculator, typename... Dependencies>
UIReadOnlyValueBinding<T> bindValue( ComputedValue<T, Calculator, Dependencies...>& value,
									 UIWidget* widget, const std::string& propertyName ) {
	return UIReadOnlyValueBinding<T>( value, widget, UIValueConverter<T>::converterDefault(),
									  propertyName );
}

/** @brief Creates a scoped one-way binding from any observable source to a widget. */
template <typename Source>
auto bindReadOnlyValue( Source& value, UIWidget* widget,
						const UIValueConverter<typename Source::ValueType>& converter =
							UIValueConverter<typename Source::ValueType>::converterDefault(),
						const std::string& propertyName = "value" ) {
	using T = typename Source::ValueType;
	return UIReadOnlyValueBinding<T>( value, widget, converter, propertyName );
}

/** @brief Creates a one-way binding to a non-default property using default conversion. */
template <typename Source>
auto bindReadOnlyValue( Source& value, UIWidget* widget, const std::string& propertyName ) {
	using T = typename Source::ValueType;
	return UIReadOnlyValueBinding<T>( value, widget, UIValueConverter<T>::converterDefault(),
									  propertyName );
}

}} // namespace EE::UI

#endif
