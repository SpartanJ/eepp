#ifndef EE_GRAPHICS_FONTSTYLECONFIG_HPP
#define EE_GRAPHICS_FONTSTYLECONFIG_HPP

#include <eepp/config.hpp>
#include <eepp/system/colors.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

class Font;

class FontStyleConfig {
	public:
		Graphics::Font * getFont() const {
			return Font;
		}

		const ColorA& getFontColor() const {
			return Color;
		}

		const ColorA& getFontShadowColor() const {
			return ShadowColor;
		}

		Uint32 getFontCharacterSize() const {
			return CharacterSize;
		}

		Uint32 getFontStyle() const {
			return Style;
		}

		Float getOutlineThickness() const {
			return OutlineThickness;
		}

		ColorA getOutlineColor() const {
			return OutlineColor;
		}

		FontStyleConfig() {}

		FontStyleConfig( const FontStyleConfig& fontStyleConfig ) :
			Font( fontStyleConfig.Font ),
			CharacterSize( fontStyleConfig.CharacterSize ),
			Style( fontStyleConfig.Style ),
			Color( fontStyleConfig.Color ),
			ShadowColor( fontStyleConfig.ShadowColor ),
			OutlineThickness( fontStyleConfig.OutlineThickness ),
			OutlineColor( fontStyleConfig.OutlineColor )
		{}

		virtual void updateFontStyleConfig( const FontStyleConfig& fontStyleConfig ) {
			Font = fontStyleConfig.Font;
			Style = fontStyleConfig.Style;
			CharacterSize = fontStyleConfig.CharacterSize;
			Color = fontStyleConfig.Color;
			ShadowColor = fontStyleConfig.ShadowColor;
			OutlineThickness = fontStyleConfig.OutlineThickness;
			OutlineColor = fontStyleConfig.OutlineColor;
		}

		Graphics::Font * Font = NULL;
		Uint32 CharacterSize = 12;
		Uint32 Style = 0;
		ColorA Color = ColorA(255,255,255,255);
		ColorA ShadowColor = ColorA(50,50,50,230);
		Float OutlineThickness = 0;
		ColorA OutlineColor = ColorA(0,0,0,255);
};

}}

#endif
