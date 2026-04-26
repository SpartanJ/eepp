#pragma once

#include <eepp/ui/doc/syntaxcolorscheme.hpp>

using namespace EE::UI::Doc;

namespace ecode {

class ColorSchemeTranslator {
  public:
	static std::string fromSyntaxColorScheme( const SyntaxColorScheme& colorScheme );
};

} // namespace ecode
