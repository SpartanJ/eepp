#ifndef EE_GRAPHICS_FONTSTYLECONFIG_HPP
#define EE_GRAPHICS_FONTSTYLECONFIG_HPP

#include <eepp/config.hpp>
#include <eepp/system/color.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

class Font;

class FontStyleConfig {
	public:
		Graphics::Font * getFont() const {
			return Font;
		}

		const Color& getFontColor() const {
			return FontColor;
		}

		const Color& getFontShadowColor() const {
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

		Color getOutlineColor() const {
			return OutlineColor;
		}

		FontStyleConfig() {}

		FontStyleConfig( const FontStyleConfig& fontStyleConfig ) :
			Font( fontStyleConfig.Font ),
			CharacterSize( fontStyleConfig.CharacterSize ),
			Style( fontStyleConfig.Style ),
			FontColor( fontStyleConfig.FontColor ),
			ShadowColor( fontStyleConfig.ShadowColor ),
			OutlineThickness( fontStyleConfig.OutlineThickness ),
			OutlineColor( fontStyleConfig.OutlineColor )
		{}

		virtual void updateFontStyleConfig( const FontStyleConfig& fontStyleConfig ) {
			Font = fontStyleConfig.Font;
			Style = fontStyleConfig.Style;
			CharacterSize = fontStyleConfig.CharacterSize;
			FontColor = fontStyleConfig.FontColor;
			ShadowColor = fontStyleConfig.ShadowColor;
			OutlineThickness = fontStyleConfig.OutlineThickness;
			OutlineColor = fontStyleConfig.OutlineColor;
		}

		Graphics::Font * Font = NULL;
		Uint32 CharacterSize = 12;
		Uint32 Style = 0;
		Color FontColor = Color(255,255,255,255);
		Color ShadowColor = Color(50,50,50,230);
		Float OutlineThickness = 0;
		Color OutlineColor = Color(0,0,0,255);
};

}}

#endif
