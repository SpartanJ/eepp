#ifndef EE_UI_UIDATABIND_HPP
#define EE_UI_UIDATABIND_HPP

#include <eepp/core/containers.hpp>
#include <eepp/core/debug.hpp>
#include <eepp/ui/uivalueconverter.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <memory>
#include <variant>

namespace EE { namespace UI {

/**
 * @brief Synchronizes a value with one or more UIWidget properties.
 *
 * UIDataBind observes each widget's value event and writes converted values back to the external
 * object supplied at construction. Calling set() updates that object and propagates the converted
 * value to every bound widget.
 *
 * The converter maps directly between T and the widget property string. Its toValue() callback
 * decides whether widget input is acceptable. Values passed to set() are authoritative model state
 * and are formatted through fromValue().
 *
 * @warning The external object is not owned. It must outlive the UIDataBind, or reset() must be
 * called before that object is destroyed. UIProperty is the owning alternative when the value
 * should have the same lifetime as its binding.
 *
 * Widgets are also observed without ownership: EventConnection handles remove listeners when the
 * binding dies, while the widget-level Event::OnClose notification removes widgets that die before
 * the binding. All binding operations and widget events must run on the widgets' owning UI thread.
 * The class is non-copyable and non-movable because its listeners capture its address.
 *
 * Use UIDataBind when adapting an existing externally owned value and its lifetime is already
 * controlled by the caller. Prefer UIProperty for small UI-local state, or ObservableValue with
 * UIValueBinding when the model must publish changes without depending on the UI.
 *
 * @code
 * bool showDetails = false;
 * auto binding = UIDataBind<bool>::New(
 *     &showDetails, checkbox, UIValueConverter<bool>::converterBool() );
 * // 'binding' must be destroyed or reset before 'showDetails'.
 * @endcode
 */
template <typename T> class UIDataBind {
  public:
	using Converter = UIValueConverter<T>;

	// Compatibility helpers keep existing UIDataBind call sites source-compatible while the
	// conversion policy itself remains independent from this binding type.
	static Converter converterDefault() { return Converter::converterDefault(); }
	static Converter converterString() { return Converter::converterString(); }
	static Converter converterBool() { return Converter::converterBool(); }

	static std::unique_ptr<UIDataBind<T>>
	New( T* t, const UnorderedSet<UIWidget*>& widgets,
		 const Converter& converter = Converter::converterDefault(),
		 const std::string& valueKey = "value",
		 const Event::EventType& eventType = Event::OnValueChange ) {
		return std::unique_ptr<UIDataBind<T>>(
			new UIDataBind<T>( t, widgets, converter, valueKey, eventType ) );
	}

	static std::unique_ptr<UIDataBind<T>>
	New( T* t, UIWidget* widget, const Converter& converter = Converter::converterDefault(),
		 const std::string& valueKey = "value",
		 const Event::EventType& eventType = Event::OnValueChange ) {
		return std::unique_ptr<UIDataBind<T>>(
			new UIDataBind<T>( t, widget, converter, valueKey, eventType ) );
	}

	UIDataBind() = default;
	UIDataBind( const UIDataBind& ) = delete;
	UIDataBind& operator=( const UIDataBind& ) = delete;
	UIDataBind( UIDataBind&& ) = delete;
	UIDataBind& operator=( UIDataBind&& ) = delete;

	UIDataBind( T* t, const UnorderedSet<UIWidget*>& widgets,
				const Converter& converter = Converter::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) {
		init( t, widgets, converter, valueKey, eventType );
	}

	UIDataBind( T* t, UIWidget* widget, const Converter& converter = Converter::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) {
		init( t, { widget }, converter, valueKey, eventType );
	}

	void init( T* t, const UnorderedSet<UIWidget*>& widgets,
			   const Converter& converter = Converter::converterDefault(),
			   const std::string& valueKey = "value",
			   const Event::EventType& eventType = Event::OnValueChange ) {
		eeASSERT( t != nullptr );
		reset();
		data = t;
		this->widgets = widgets;
		this->property = StyleSheetSpecification::instance()->getProperty( valueKey );
		this->converter = converter;
		this->eventType = eventType;
		for ( auto widget : widgets ) {
			eeASSERT( widget != nullptr );
			bindListeners( widget );
		}
		set( *data );
		dataInitialized = true;
	}

	/** Propagates the authoritative model value and reports formatting failures. */
	UIValueValidationResult set( const T& t ) { return setData( t ); }

	/** Propagates the authoritative model value and reports formatting failures. */
	UIValueValidationResult set( T&& t ) { return setData( std::move( t ) ); }

	const T& get() const {
		eeASSERT( isInitialized() );
		return *data;
	}

	/** @return True when the binding has a valid external value, property, and converter. */
	bool isInitialized() const {
		return data != nullptr && property != nullptr && converter.toValue && converter.fromValue;
	}

	/**
	 * @brief Disconnects every widget and releases the reference to the external value.
	 *
	 * After reset(), the binding must be initialized again before get() or set() is used.
	 */
	void reset() {
		connections.clear();
		widgets.clear();
		converter = Converter();
		validation.clear();
		validationEmitter = nullptr;
		inSetValue = false;
		dataInitialized = false;
		property = nullptr;
		data = nullptr;
	}

	/** @brief Adds @p widget to the synchronized widget set. Duplicate binds are ignored. */
	void bind( UIWidget* widget ) {
		eeASSERT( isInitialized() );
		eeASSERT( widget != nullptr );
		if ( widgets.find( widget ) != widgets.end() )
			return;
		bindListeners( widget );
		widgets.insert( widget );
		std::string string;
		auto result = dataToString( string );
		if ( result ) {
			inSetValue = true;
			widget->applyProperty( StyleSheetProperty( property, string ) );
			inSetValue = false;
		}
		setValidationResult( std::move( result ) );
	}

	/** @brief Disconnects and removes @p widget from the synchronized widget set. */
	void unbind( UIWidget* widget ) {
		if ( widgets.find( widget ) == widgets.end() )
			return;
		connections.erase( widget );
		widgets.erase( widget );
		if ( validationEmitter == widget ) {
			validationEmitter = nullptr;
			validation.clear();
		}
	}

	~UIDataBind() {
		// Do not publish a final "valid" transition while the binding itself is being destroyed.
		// Validation connections expire safely with validation after widget listeners are removed.
		connections.clear();
		widgets.clear();
	}

	const PropertyDefinition* getPropertyDefinition() const { return property; }

	std::function<void( const T& newVal )> onValueChangeCb;

	const UnorderedSet<UIWidget*>& getWidgets() const { return widgets; }

	/** @return Observable converter error state for this binding. */
	UIValueValidationState& validationState() { return validation; }
	const UIValueValidationState& validationState() const { return validation; }
	bool isValid() const { return validation.isValid(); }

  protected:
	template <typename U> UIValueValidationResult setData( U&& t ) {
		eeASSERT( isInitialized() );
		if ( dataInitialized && t == *data ) {
			inSetValue = true;
			auto result = setValueChange();
			inSetValue = false;
			setValidationResult( result );
			return result;
		}
		inSetValue = true;
		*data = std::forward<U>( t );
		auto result = setValueChange();
		inSetValue = false;
		if ( onValueChangeCb )
			onValueChangeCb( *data );
		setValidationResult( result );
		return result;
	}

	T* data{ nullptr };
	UnorderedSet<UIWidget*> widgets;
	UnorderedMap<UIWidget*, EventConnectionList> connections;
	bool inSetValue{ false };
	bool dataInitialized{ false };
	const PropertyDefinition* property{ nullptr };
	Converter converter;
	UIValueValidationState validation;
	UIWidget* validationEmitter{ nullptr };
	Event::EventType eventType{ Event::OnValueChange };

	void bindListeners( UIWidget* widget ) {
		auto& widgetConnections = connections[widget];
		widgetConnections += widget->connect( eventType, [this]( const Event* event ) {
			processValueChange( event->getNode()->asType<UIWidget>() );
		} );
		widgetConnections += widget->connect( Event::OnClose, [this]( const Event* event ) {
			auto widget = event->getNode()->asType<UIWidget>();
			connections.erase( widget );
			widgets.erase( widget );
			if ( validationEmitter == widget ) {
				validationEmitter = nullptr;
				validation.clear();
			}
		} );
	}

	UIValueValidationResult dataToString( std::string& string ) const {
		eeASSERT( isInitialized() );
		auto converted = converter.fromValue( property, *data );
		if ( !converted )
			return converted.validation;
		string = std::move( *converted.value );
		return UIValueValidationResult::success();
	}

	void setValidationResult( UIValueValidationResult result, UIWidget* emitter = nullptr ) {
		validationEmitter = result ? nullptr : emitter;
		validation.set( std::move( result ) );
	}

	void processValueChange( UIWidget* emitter ) {
		eeASSERT( isInitialized() );
		eeASSERT( emitter != nullptr );
		if ( inSetValue )
			return;
		auto proposed = converter.toValue( property, emitter->getPropertyString( property ) );
		if ( !proposed ) {
			setValidationResult( std::move( proposed.validation ), emitter );
			return;
		}

		auto canonicalString = converter.fromValue( property, *proposed.value );
		if ( !canonicalString ) {
			setValidationResult( std::move( canonicalString.validation ), emitter );
			return;
		}
		*data = std::move( *proposed.value );
		StyleSheetProperty prop( property, *canonicalString.value, 0, false );
		inSetValue = true;
		for ( auto widget : widgets ) {
			if ( widget != emitter )
				widget->applyProperty( prop );
		}
		inSetValue = false;
		validationEmitter = nullptr;
		validation.clear();
		if ( onValueChangeCb )
			onValueChangeCb( *data );
	}

	UIValueValidationResult setValueChange() {
		std::string string;
		auto result = dataToString( string );
		if ( !result )
			return result;
		StyleSheetProperty prop( property, string, 0, false );
		for ( auto widget : widgets )
			widget->applyProperty( prop );
		return result;
	}
};

class UIDataBindBool {
  public:
	using Ptr = std::unique_ptr<UIDataBind<bool>>;

	static Ptr
	New( bool* t, const UnorderedSet<UIWidget*>& widgets,
		 const UIDataBind<bool>::Converter& converter = UIDataBind<bool>::converterBool(),
		 const std::string& valueKey = "value" ) {
		return UIDataBind<bool>::New( t, widgets, converter, valueKey );
	}

	static Ptr
	New( bool* t, UIWidget* widget,
		 const UIDataBind<bool>::Converter& converter = UIDataBind<bool>::converterBool(),
		 const std::string& valueKey = "value" ) {
		return UIDataBind<bool>::New( t, widget, converter, valueKey );
	}
};

class UIDataBindString {
  public:
	using Ptr = std::unique_ptr<UIDataBind<std::string>>;

	static Ptr New( std::string* t, const UnorderedSet<UIWidget*>& widgets,
					const UIDataBind<std::string>::Converter& converter =
						UIDataBind<std::string>::converterString(),
					const std::string& valueKey = "text",
					const Event::EventType& eventType = Event::OnTextChanged ) {
		return UIDataBind<std::string>::New( t, widgets, converter, valueKey, eventType );
	}

	static Ptr New( std::string* t, UIWidget* widget,
					const UIDataBind<std::string>::Converter& converter =
						UIDataBind<std::string>::converterString(),
					const std::string& valueKey = "text",
					const Event::EventType& eventType = Event::OnTextChanged ) {
		return UIDataBind<std::string>::New( t, widget, converter, valueKey, eventType );
	}
};

/**
 * @brief Owns heterogeneous UIDataBind instances with stable heap addresses.
 *
 * Clearing or destroying the holder destroys every binding and disconnects its widget listeners.
 */
template <typename... Ts> class UIDataBindHolder {
  public:
	using UIDataBindVariant = std::variant<std::unique_ptr<UIDataBind<Ts>>...>;

	UIDataBindHolder& hold( UIDataBindVariant&& ptr ) {
		mHolder.emplace_back( std::move( ptr ) );
		return *this;
	}

	UIDataBindHolder& operator+=( UIDataBindVariant&& ptr ) {
		mHolder.emplace_back( std::move( ptr ) );
		return *this;
	}

	void clear() { mHolder.clear(); }

  protected:
	std::vector<UIDataBindVariant> mHolder;
};

/**
 * @brief Keyed owner for heterogeneous UIDataBind instances with stable heap addresses.
 *
 * Replacing a key destroys its previous binding and disconnects that binding's widget listeners.
 */
template <typename... Ts> class UIDataBindHolderKV {
  public:
	using UIDataBindVariant = std::variant<std::unique_ptr<UIDataBind<Ts>>...>;

	UIDataBindHolderKV& hold( std::string key, UIDataBindVariant&& ptr ) {
		mHolder[std::move( key )] = std::move( ptr );
		return *this;
	}

	UIDataBindHolderKV& operator+=( std::pair<std::string, UIDataBindVariant&&> pair ) {
		mHolder[std::move( pair.first )] = std::move( pair.second );
		return *this;
	}

	void clear() { mHolder.clear(); }

  protected:
	UnorderedMap<std::string, UIDataBindVariant> mHolder;
};

}} // namespace EE::UI

#endif // EE_UI_UIDATABIND_HPP
