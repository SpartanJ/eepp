#ifndef EE_UICUITHEMECONFIG_HPP
#define EE_UICUITHEMECONFIG_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/graphics/fontstyleconfig.hpp>

namespace EE { namespace Graphics {
class Font;
}}

namespace EE { namespace UI {

class UIFontStyleConfig : public FontStyleConfig {
	public:
		const Color& getFontSelectedColor() const {
			return FontSelectedColor;
		}

		Color getFontSelectionBackColor() const {
			return FontSelectionBackColor;
		}

		void setFontSelectedColor( const Color& color ) {
			FontSelectedColor = color;
		}

		void setFontSelectionBackColor(const Color& color) {
			FontSelectionBackColor = color;
		}

		UIFontStyleConfig() : FontStyleConfig() {}

		UIFontStyleConfig( const UIFontStyleConfig& fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig ),
			FontSelectedColor( fontStyleConfig.FontSelectedColor ),
			FontSelectionBackColor( fontStyleConfig.FontSelectionBackColor )
		{}

		virtual void updateStyleConfig( const UIFontStyleConfig& fontStyleConfig ) {
			FontStyleConfig::updateFontStyleConfig( fontStyleConfig );
			FontSelectedColor = fontStyleConfig.FontSelectedColor;
			FontSelectionBackColor = fontStyleConfig.FontSelectionBackColor;
		}

		Color FontSelectedColor = Color(255,255,255,255);
		Color FontSelectionBackColor = Color(50,50,50,255);
};

}}

#endif
