#ifndef EE_UICUITHEMECONFIG_HPP
#define EE_UICUITHEMECONFIG_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uihelper.hpp>

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

		Int32		tabSeparation = 0;
		Uint32		maxTextLength = 30;
		Uint32		tabWidgetHeight = 0;
		Uint32		tabTextAlign = ( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
		Uint32		minTabWidth = 32;
		Uint32		maxTabWidth = 300;
		bool		tabsClosable = false;
		bool		specialBorderTabs = false; //! Indicates if the periferical tabs ( the left and right border tab ) are different from the central tabs.
		bool		drawLineBelowTabs = false;
		ColorA		lineBelowTabsColor;
		Int32		lineBelowTabsYOffset = 0;
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

class WinMenuStyleConfig : public FontStyleConfig {
	public:
		WinMenuStyleConfig() {}

		WinMenuStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		Uint32				marginBetweenButtons = 0;
		Uint32				buttonMargin = 4;
		Uint32				menuHeight = 0;
		Uint32				firstButtonMargin = 1;
};

class DropDownListStyleConfig : public FontStyleConfig {
	public:
		DropDownListStyleConfig() {}

		DropDownListStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		Uint32 maxNumVisibleItems = 10;
		bool popUpToMainControl = false;
};

class WindowStyleConfig : public FontStyleConfig {
	public:
		WindowStyleConfig() {}

		WindowStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		Uint32		winFlags = UI_WIN_DEFAULT_FLAGS;
		Sizei		decorationSize;
		Sizei		borderSize;
		Sizei		minWindowSize;
		Vector2i	buttonsPositionFixer;
		Uint32		buttonsSeparation = 4;
		Int32		minCornerDistance = 24;
		ColorA		titleFontColor = ColorA( 255, 255, 255, 255 );
		Uint8		baseAlpha = 255;
		bool		decorationAutoSize = true;
		bool		borderAutoSize = true;
};

}}

#endif
