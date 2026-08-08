#ifndef EE_UI_UIVALUECONVERTER_HPP
#define EE_UI_UIVALUECONVERTER_HPP

#include <eepp/core/string.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace EE { namespace UI {

/**
 * @brief Binding-independent conversion between a typed value and a widget property string.
 *
 * UIDataBind and UIValueBinding share this policy type, so custom conversions do not depend on
 * either ownership model. The PropertyDefinition describes the widget property being synchronized
 * and can be used for CSS-aware parsing.
 *
 * @code
 * auto converter = UIValueConverter<MyEnum>(
 *     []( const CSS::PropertyDefinition*, MyEnum& value, const std::string& text ) {
 *         return enumFromString( value, text );
 *     },
 *     []( const CSS::PropertyDefinition*, std::string& text, const MyEnum& value ) {
 *         text = enumToString( value );
 *         return true;
 *     } );
 * @endcode
 */
template <typename T> struct UIValueConverter {
	using ToValue = std::function<bool( const CSS::PropertyDefinition*, T&, const std::string& )>;
	using FromValue = std::function<bool( const CSS::PropertyDefinition*, std::string&, const T& )>;

	UIValueConverter() = default;
	UIValueConverter( ToValue toValue, FromValue fromValue ) :
		toValue( std::move( toValue ) ), fromValue( std::move( fromValue ) ) {}

	ToValue toValue;
	FromValue fromValue;

	static UIValueConverter converterDefault() {
		return UIValueConverter(
			[]( const CSS::PropertyDefinition* property, T& value, const std::string& string ) {
				if constexpr ( std::is_same_v<T, std::string> || std::is_same_v<T, String> ) {
					value = T( string );
					return true;
				} else if constexpr ( std::is_same_v<T, bool> ) {
					value = CSS::StyleSheetProperty( property, string ).asBool();
					return true;
				} else {
					return String::fromString( value, string );
				}
			},
			[]( const CSS::PropertyDefinition*, std::string& string, const T& value ) {
				if constexpr ( std::is_same_v<T, std::string> ) {
					string = value;
				} else if constexpr ( std::is_same_v<T, String> ) {
					string = value.toUtf8();
				} else if constexpr ( std::is_same_v<T, double> ) {
					string = String::fromDouble( value );
				} else if constexpr ( std::is_same_v<T, float> ) {
					string = String::fromFloat( value );
				} else if constexpr ( std::is_same_v<T, bool> ) {
					string = value ? "true" : "false";
				} else {
					string = String::toString( value );
				}
				return true;
			} );
	}

	static UIValueConverter converterString() {
		return UIValueConverter(
			[]( const CSS::PropertyDefinition*, T& value, const std::string& string ) {
				value = T( string );
				return true;
			},
			[]( const CSS::PropertyDefinition*, std::string& string, const T& value ) {
				if constexpr ( std::is_same_v<T, String> )
					string = value.toUtf8();
				else
					string = value;
				return true;
			} );
	}

	static UIValueConverter converterBool() {
		return UIValueConverter(
			[]( const CSS::PropertyDefinition* property, T& value, const std::string& string ) {
				value = CSS::StyleSheetProperty( property, string ).asBool();
				return true;
			},
			[]( const CSS::PropertyDefinition*, std::string& string, const T& value ) {
				string = value ? "true" : "false";
				return true;
			} );
	}
};

}} // namespace EE::UI

#endif
