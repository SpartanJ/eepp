#ifndef EE_UI_TOOLS_HTMLFORMATTER_HPP
#define EE_UI_TOOLS_HTMLFORMATTER_HPP

#include <eepp/config.hpp>
#include <string>

namespace EE { namespace UI { namespace Tools {

class EE_API HTMLFormatter {
  public:
	static std::string HTMLtoXML( const std::string& layoutString );
};

}}} // namespace EE::UI::Tools

#endif
