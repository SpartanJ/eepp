#ifndef EE_UI_UIVALUECONVERTER_HPP
#define EE_UI_UIVALUECONVERTER_HPP

#include <eepp/core/string.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/uivaluevalidation.hpp>
#include <functional>
#include <string>
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
 *     []( const CSS::PropertyDefinition*, const std::string& text ) {
 *         MyEnum value;
 *         return enumFromString( value, text ) ? UIValueResult<MyEnum>::success( value )
 *                                               : UIValueResult<MyEnum>::error( 1 );
 *     },
 *     []( const CSS::PropertyDefinition*, const MyEnum& value ) {
 *         return UIValueResult<std::string>::success( enumToString( value ) );
 *     } );
 * @endcode
 */
template <typename T> struct UIValueConverter {
	using ToValue =
		std::function<UIValueResult<T>( const CSS::PropertyDefinition*, const std::string& )>;
	using FromValue =
		std::function<UIValueResult<std::string>( const CSS::PropertyDefinition*, const T& )>;

	UIValueConverter() = default;
	UIValueConverter( ToValue toValue, FromValue fromValue ) :
		toValue( std::move( toValue ) ), fromValue( std::move( fromValue ) ) {}

	ToValue toValue;
	FromValue fromValue;

	static UIValueConverter converterDefault() {
		return UIValueConverter(
			[]( const CSS::PropertyDefinition* property, const std::string& string ) {
				if constexpr ( std::is_same_v<T, std::string> || std::is_same_v<T, String> ) {
					return UIValueResult<T>::success( T( string ) );
				} else if constexpr ( std::is_same_v<T, bool> ) {
					return UIValueResult<T>::success(
						CSS::StyleSheetProperty( property, string ).asBool() );
				} else {
					T value;
					return String::fromString( value, string )
							   ? UIValueResult<T>::success( std::move( value ) )
							   : UIValueResult<T>::error( static_cast<Uint32>(
									 UIValueValidationError::ConversionFailed ) );
				}
			},
			[]( const CSS::PropertyDefinition*, const T& value ) {
				std::string string;
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
				return UIValueResult<std::string>::success( std::move( string ) );
			} );
	}

	static UIValueConverter converterString() {
		return UIValueConverter(
			[]( const CSS::PropertyDefinition*, const std::string& string ) {
				return UIValueResult<T>::success( T( string ) );
			},
			[]( const CSS::PropertyDefinition*, const T& value ) {
				if constexpr ( std::is_same_v<T, String> )
					return UIValueResult<std::string>::success( value.toUtf8() );
				else
					return UIValueResult<std::string>::success( value );
			} );
	}

	static UIValueConverter converterBool() {
		return UIValueConverter(
			[]( const CSS::PropertyDefinition* property, const std::string& string ) {
				return UIValueResult<T>::success(
					CSS::StyleSheetProperty( property, string ).asBool() );
			},
			[]( const CSS::PropertyDefinition*, const T& value ) {
				return UIValueResult<std::string>::success( value ? "true" : "false" );
			} );
	}
};

}} // namespace EE::UI

#endif
