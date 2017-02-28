#ifndef EE_UICUITHEMECONFIG_HPP
#define EE_UICUITHEMECONFIG_HPP

#include <eepp/ui/base.hpp>

namespace EE { namespace Graphics {
class Font;
}}

namespace EE { namespace UI {

class FontStyleConfig {
	public:
		Graphics::Font * getFont() const {
			return font;
		}

		const ColorA& getFontColor() const {
			return fontColor;
		}

		const ColorA& getFontShadowColor() const {
			return fontShadowColor;
		}

		const ColorA& getFontOverColor() const {
			return fontOverColor;
		}

		const ColorA& getFontSelectedColor() const {
			return fontSelectedColor;
		}

		ColorA getFontSelectionBackColor() const {
			return fontSelectionBackColor;
		}

		void setFont( Font * font ) {
			this->font = font;
		}

		void setFontColor( const ColorA& color ) {
			fontColor = color;
		}

		void setFontShadowColor( const ColorA& color ) {
			fontShadowColor = color;
		}

		void setFontOverColor( const ColorA& color ) {
			fontOverColor = color;
		}

		void setFontSelectedColor( const ColorA& color ) {
			fontSelectedColor = color;
		}

		void setFontSelectionBackColor(const ColorA& color) {
			fontSelectionBackColor = color;
		}

		FontStyleConfig() {}

		FontStyleConfig( const FontStyleConfig& fontStyleConfig ) :
			font( fontStyleConfig.font ),
			fontColor( fontStyleConfig.fontColor ),
			fontShadowColor( fontStyleConfig.fontShadowColor ),
			fontOverColor( fontStyleConfig.fontOverColor ),
			fontSelectedColor( fontStyleConfig.fontSelectedColor ),
			fontSelectionBackColor( fontStyleConfig.fontSelectionBackColor )
		{}

		void updateFontStyleConfig( const FontStyleConfig& fontStyleConfig ) {
			font = ( fontStyleConfig.font );
			fontColor = ( fontStyleConfig.fontColor );
			fontShadowColor = ( fontStyleConfig.fontShadowColor );
			fontOverColor = ( fontStyleConfig.fontOverColor );
			fontSelectedColor = ( fontStyleConfig.fontSelectedColor );
			fontSelectionBackColor = ( fontStyleConfig.fontSelectionBackColor );
		}

		Font * font;
		ColorA fontColor;
		ColorA fontShadowColor;
		ColorA fontOverColor;
		ColorA fontSelectedColor;
		ColorA fontSelectionBackColor;
};

class TabWidgetStyleConfig : public FontStyleConfig {
	public:
		TabWidgetStyleConfig() {}

		TabWidgetStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		Int32		tabSeparation;
		Uint32		maxTextLength;
		Uint32		tabWidgetHeight;
		Uint32		tabTextAlign;
		Uint32		minTabWidth;
		Uint32		maxTabWidth;
		bool		tabsClosable;
		bool		specialBorderTabs; //! Indicates if the periferical tabs ( the left and right border tab ) are different from the central tabs.
		bool		drawLineBelowTabs;
		ColorA		lineBelowTabsColor;
		Int32		lineBelowTabsYOffset;
};

class ProgressBarStyleConfig : public FontStyleConfig {
	public:
		ProgressBarStyleConfig() {}

		ProgressBarStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		bool displayPercent = false;
		bool verticalExpand = true;
		Vector2f movementSpeed = Vector2f( 64.f, 0 );
		Rectf fillerPadding;
};

}}

#endif
