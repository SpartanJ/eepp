#ifndef EE_UI_DOC_MARKDOWNHELPER_HPP
#define EE_UI_DOC_MARKDOWNHELPER_HPP

#include <eepp/config.hpp>
#include <string>
#include <string_view>

namespace EE { namespace UI { namespace Doc {

class EE_API Markdown {
  public:
	static std::string toXHTML( std::string_view markdown );
};

}}} // namespace EE::UI::Doc

#endif
