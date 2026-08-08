#ifndef EE_UI_UIDATABIND_HPP
#define EE_UI_UIDATABIND_HPP

#include <eepp/core/containers.hpp>
#include <eepp/core/debug.hpp>
#include <eepp/system/log.hpp>
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
		 const Converter& converter = UIDataBind<T>::converterDefault(),
		 const std::string& valueKey = "value",
		 const Event::EventType& eventType = Event::OnValueChange ) {
		return std::unique_ptr<UIDataBind<T>>(
			new UIDataBind<T>( t, widgets, converter, valueKey, eventType ) );
	}

	static std::unique_ptr<UIDataBind<T>>
	New( T* t, UIWidget* widget, const Converter& converter = UIDataBind<T>::converterDefault(),
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
				const Converter& converter = UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) {
		init( t, widgets, converter, valueKey, eventType );
	}

	UIDataBind( T* t, UIWidget* widget,
				const Converter& converter = UIDataBind<T>::converterDefault(),
				const std::string& valueKey = "value",
				const Event::EventType& eventType = Event::OnValueChange ) {
		init( t, { widget }, converter, valueKey, eventType );
	}

	void init( T* t, const UnorderedSet<UIWidget*>& widgets,
			   const Converter& converter = UIDataBind<T>::converterDefault(),
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

	void set( const T& t ) {
		eeASSERT( isInitialized() );
		if ( dataInitialized && t == *data )
			return;
		inSetValue = true;
		*data = t;
		setValueChange();
		inSetValue = false;
		if ( onValueChangeCb )
			onValueChangeCb( t );
	}

	void set( T&& t ) {
		eeASSERT( isInitialized() );
		if ( dataInitialized && t == *data )
			return;
		inSetValue = true;
		*data = std::move( t );
		setValueChange();
		inSetValue = false;
		if ( onValueChangeCb )
			onValueChangeCb( *data );
	}

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
		inSetValue = true;
		widget->applyProperty( StyleSheetProperty( property, dataToString() ) );
		inSetValue = false;
	}

	/** @brief Disconnects and removes @p widget from the synchronized widget set. */
	void unbind( UIWidget* widget ) {
		if ( widgets.find( widget ) == widgets.end() )
			return;
		connections.erase( widget );
		widgets.erase( widget );
	}

	~UIDataBind() { reset(); }

	const PropertyDefinition* getPropertyDefinition() const { return property; }

	std::function<void( const T& newVal )> onValueChangeCb;

	const UnorderedSet<UIWidget*>& getWidgets() const { return widgets; }

  protected:
	T* data{ nullptr };
	UnorderedSet<UIWidget*> widgets;
	UnorderedMap<UIWidget*, EventConnectionList> connections;
	bool inSetValue{ false };
	bool dataInitialized{ false };
	const PropertyDefinition* property{ nullptr };
	Converter converter;
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
		} );
	}

	std::string dataToString() const {
		eeASSERT( isInitialized() );
		std::string str;
		if ( !converter.fromValue( property, str, *data ) ) {
			Log::error( "UIDataBind::dataToString converter::fromValue: unable to convert value "
						"to string." );
		}
		return str;
	}

	void processValueChange( UIWidget* emitter ) {
		eeASSERT( isInitialized() );
		eeASSERT( emitter != nullptr );
		if ( inSetValue )
			return;
		bool success = false;
		T val;
		success = converter.toValue( property, val, emitter->getPropertyString( property ) );

		if ( success ) {
			*data = val;
			StyleSheetProperty prop( property, dataToString(), 0, false );
			inSetValue = true;
			for ( auto widget : widgets ) {
				if ( widget != emitter )
					widget->applyProperty( prop );
			}
			inSetValue = false;
			if ( onValueChangeCb )
				onValueChangeCb( val );
		}
	}

	void setValueChange() {
		StyleSheetProperty prop( property, dataToString(), 0, false );
		for ( auto widget : widgets )
			widget->applyProperty( prop );
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
