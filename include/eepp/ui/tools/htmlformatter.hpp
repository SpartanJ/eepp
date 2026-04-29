#ifndef EE_UI_TOOLS_HTMLFORMATTER_HPP
#define EE_UI_TOOLS_HTMLFORMATTER_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>

namespace pugi {
class xml_node;
}

namespace EE { namespace UI { namespace Tools {

class EE_API HTMLFormatter {
  public:
	static bool isInlineNode( const pugi::xml_node& node );

	static bool hasSignificantText( const pugi::xml_node& node );

	static pugi::xml_node getLogicalPrev( const pugi::xml_node& node );

	static pugi::xml_node getLogicalNext( const pugi::xml_node& node );

	static String collapseXmlWhitespace( const String& text, const pugi::xml_node& node );

	static std::string HTMLtoXML( const std::string& layoutString );
};

}}} // namespace EE::UI::Tools

#endif
