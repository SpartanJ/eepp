#ifndef EE_GRAPHICS_FONTSTYLECONFIG_HPP
#define EE_GRAPHICS_FONTSTYLECONFIG_HPP

#include <eepp/config.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/system/color.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

class Font;

class FontStyleConfig {
  public:
	Graphics::Font* getFont() const { return Font; }

	const Color& getFontColor() const { return FontColor; }

	const Color& getFontShadowColor() const { return ShadowColor; }

	const Float& getFontCharacterSize() const { return CharacterSize; }

	const Uint32& getFontStyle() const { return Style; }

	const Float& getOutlineThickness() const { return OutlineThickness; }

	const Color& getOutlineColor() const { return OutlineColor; }

	const Vector2f& getFontShadowOffset() const { return ShadowOffset; }

	FontStyleConfig() {}

	bool operator==( const FontStyleConfig& other ) {
		return Font == other.Font && CharacterSize == other.CharacterSize && Style == other.Style &&
			   FontColor == other.FontColor && ShadowColor == other.ShadowColor &&
			   ShadowOffset == other.ShadowOffset && OutlineThickness == other.OutlineThickness &&
			   OutlineColor == other.OutlineColor;
	}

	virtual void updateFontStyleConfig( const FontStyleConfig& fontStyleConfig ) {
		Font = fontStyleConfig.Font;
		Style = fontStyleConfig.Style;
		CharacterSize = fontStyleConfig.CharacterSize;
		FontColor = fontStyleConfig.FontColor;
		ShadowColor = fontStyleConfig.ShadowColor;
		ShadowOffset = fontStyleConfig.ShadowOffset;
		OutlineThickness = fontStyleConfig.OutlineThickness;
		OutlineColor = fontStyleConfig.OutlineColor;
	}

	Graphics::Font* Font{ nullptr };
	Float CharacterSize{ 12 };
	Uint32 Style{ 0 };
	Color FontColor{ 255, 255, 255, 255 };
	Color ShadowColor{ 50, 50, 50, 230 };
	Vector2f ShadowOffset{ PixelDensity::dpToPx( 1 ), PixelDensity::dpToPx( 1 ) };
	Float OutlineThickness{ 0 };
	Color OutlineColor{ 0, 0, 0, 255 };
};

}} // namespace EE::Graphics

#endif
