#pragma once

#include <eepp/ui/colorschemepreferences.hpp>
#include <eepp/ui/doc/syntaxcolorscheme.hpp>

using namespace EE::UI::Doc;

namespace ecode {

class ColorSchemeTranslator {
  public:
	static std::string fromSyntaxColorScheme( const SyntaxColorScheme& colorScheme );

	static EE::UI::ColorSchemePreference
	getColorSchemePreference( const SyntaxColorScheme& colorScheme );

  private:
	static Color getFontColor( const SyntaxColorScheme& colorScheme );

	static Color ensureTextContrast( Color foreground, const Color& background );
};

} // namespace ecode
