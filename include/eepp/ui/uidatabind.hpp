#ifndef EE_UI_UIDATABIND_HPP
#define EE_UI_UIDATABIND_HPP

#include <eepp/ui/uiwidget.hpp>
#include <set>

namespace EE { namespace UI {

template <typename T> class UIDataBind {
  public:
	struct Converter {
		Converter(
			std::function<bool( const UIDataBind<T>*, T&, const std::string& )> toVal =
				[]( const UIDataBind<T>*, T& val, const std::string& str ) {
					return String::fromString( val, str );
				},
			std::function<bool( const UIDataBind<T>*, std::string&, const T& )> fromVal =
				[]( const UIDataBind<T>*, std::string& str, const T& val ) {
					str = String::toString( val );
					return true;
				} ) :
			toVal( toVal ), fromVal( fromVal ) {}
		std::function<bool( const UIDataBind<T>*, T&, const std::string& )> toVal;
		std::function<bool( const UIDataBind<T>*, std::string&, const T& )> fromVal;
	};

	static Converter converterBool() {
		return Converter(
			[]( const UIDataBind<T>* databind, T& val, const std::string& str ) -> bool {
				val = StyleSheetProperty( databind->getPropertyDefinition(), str ).asBool();
				return true;
			},
			[]( const UIDataBind<T>*, std::string& str, const T& val ) -> bool {
				str = val ? "true" : "false";
				return true;
			} );
	}

	UIDataBind( T* t, const std::set<UIWidget*>& widgets, const Converter& converter = {},
				const std::string& valueKey = "value" ) :
		data( t ),
		widgets( widgets ),
		property( StyleSheetSpecification::instance()->getProperty( valueKey ) ),
		converter( converter ) {
		for ( auto widget : widgets )
			bindListeners( widget );
		set( *data );
	}

	UIDataBind( T* t, UIWidget* widget, const Converter& converter = {},
				const std::string& valueKey = "value" ) :
		data( t ),
		widgets( { widget } ),
		property( StyleSheetSpecification::instance()->getProperty( valueKey ) ),
		converter( converter ) {
		for ( auto widget : widgets )
			bindListeners( widget );
		set( *data );
	}

	void set( const T& t ) {
		inSetValue = true;
		*data = t;
		setValueChange();
		inSetValue = false;
	}

	const T& get() const { return *data; }

	void bind( UIWidget* widget ) {
		bindListeners( widget );
		widgets.insert( widget );
		inSetValue = true;
		widget->applyProperty( StyleSheetProperty( property, String::toString( data ) ) );
		inSetValue = false;
	}

	void unbind( UIWidget* widget ) {
		if ( widgets.find( widget ) == widgets.end() )
			return;
		widget->removeEventListener( valueCbs[widget] );
		widget->removeEventListener( closeCbs[widget] );
		valueCbs.erase( widget );
		closeCbs.erase( widget );
		widgets.erase( widget );
	}

	~UIDataBind() {
		for ( auto widget : widgets ) {
			widget->removeEventListener( valueCbs[widget] );
			widget->removeEventListener( closeCbs[widget] );
		}
	}

	const PropertyDefinition* getPropertyDefinition() const { return property; }

  protected:
	T* data;
	std::set<UIWidget*> widgets;
	std::map<UIWidget*, Uint32> valueCbs;
	std::map<UIWidget*, Uint32> closeCbs;
	bool inSetValue{ false };
	const PropertyDefinition* property{ nullptr };
	Converter converter;

	void bindListeners( UIWidget* widget ) {
		valueCbs[widget] =
			widget->addEventListener( Event::OnValueChange, [this]( const Event* event ) {
				processValueChange( event->getNode()->asType<UIWidget>() );
			} );
		closeCbs[widget] = widget->addEventListener( Event::OnClose, [this]( const Event* event ) {
			closeCbs.erase( event->getNode()->asType<UIWidget>() );
			this->widgets.erase( event->getNode()->asType<UIWidget>() );
		} );
	}

	std::string dataToString() const {
		std::string str;
		if ( !converter.fromVal( this, str, *data ) ) {
			Log::error( "UIDataBind::dataToString converter::fromVal: unable to convert value "
						"to string." );
		}
		return str;
	}

	void processValueChange( UIWidget* emitter ) {
		if ( inSetValue )
			return;
		bool success = false;
		T val;
		success = converter.toVal( this, val, emitter->getPropertyString( property ) );

		if ( success ) {
			*data = val;
			StyleSheetProperty prop( property, dataToString() );
			inSetValue = true;
			for ( auto widget : widgets ) {
				if ( widget != emitter )
					widget->applyProperty( prop );
			}
			inSetValue = false;
		}
	}

	void setValueChange() {
		StyleSheetProperty prop( property, dataToString() );
		for ( auto widget : widgets )
			widget->applyProperty( prop );
	}
};

class UIDataBindBool : public UIDataBind<bool> {
  public:
	UIDataBindBool( bool* t, const std::set<UIWidget*>& widgets,
					const std::string& valueKey = "value" ) :
		UIDataBind<bool>( t, widgets, UIDataBind<bool>::converterBool(), valueKey ) {}

	UIDataBindBool( bool* t, UIWidget* widget, const std::string& valueKey = "value" ) :
		UIDataBind<bool>( t, widget, UIDataBind<bool>::converterBool(), valueKey ) {}
};

}} // namespace EE::UI

#endif // EE_UI_UIDATABIND_HPP
