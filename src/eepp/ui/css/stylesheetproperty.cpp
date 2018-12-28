#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/core/string.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheetProperty::StyleSheetProperty()
{}

StyleSheetProperty::StyleSheetProperty( const std::string& name, const std::string& value ) :
	name( String::trim( name ) ),
	value( String::trim( value ) )
{}

}}}
