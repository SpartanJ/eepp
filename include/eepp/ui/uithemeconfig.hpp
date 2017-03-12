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
			return Font;
		}

		const ColorA& getFontColor() const {
			return FontColor;
		}

		const ColorA& getFontShadowColor() const {
			return FontShadowColor;
		}

		const ColorA& getFontOverColor() const {
			return FontOverColor;
		}

		const ColorA& getFontSelectedColor() const {
			return FontSelectedColor;
		}

		ColorA getFontSelectionBackColor() const {
			return FontSelectionBackColor;
		}

		void setFont( Font * font ) {
			Font = font;
		}

		void setFontColor( const ColorA& color ) {
			FontColor = color;
		}

		void setFontShadowColor( const ColorA& color ) {
			FontShadowColor = color;
		}

		void setFontOverColor( const ColorA& color ) {
			FontOverColor = color;
		}

		void setFontSelectedColor( const ColorA& color ) {
			FontSelectedColor = color;
		}

		void setFontSelectionBackColor(const ColorA& color) {
			FontSelectionBackColor = color;
		}

		FontStyleConfig() {}

		FontStyleConfig( const FontStyleConfig& fontStyleConfig ) :
			Font( fontStyleConfig.Font ),
			FontColor( fontStyleConfig.FontColor ),
			FontShadowColor( fontStyleConfig.FontShadowColor ),
			FontOverColor( fontStyleConfig.FontOverColor ),
			FontSelectedColor( fontStyleConfig.FontSelectedColor ),
			FontSelectionBackColor( fontStyleConfig.FontSelectionBackColor )
		{}

		void updateFontStyleConfig( const FontStyleConfig& fontStyleConfig ) {
			Font = fontStyleConfig.Font ;
			FontColor = fontStyleConfig.FontColor ;
			FontShadowColor = fontStyleConfig.FontShadowColor ;
			FontOverColor = fontStyleConfig.FontOverColor ;
			FontSelectedColor = fontStyleConfig.FontSelectedColor ;
			FontSelectionBackColor = fontStyleConfig.FontSelectionBackColor ;
		}

		Graphics::Font * Font;
		Uint32 FontCharacterSize;
		ColorA FontColor;
		ColorA FontShadowColor;
		ColorA FontOverColor;
		ColorA FontSelectedColor;
		ColorA FontSelectionBackColor;
};

class TabWidgetStyleConfig : public FontStyleConfig {
	public:
		TabWidgetStyleConfig() {}

		TabWidgetStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		Int32		TabSeparation = 0;
		Uint32		MaxTextLength = 30;
		Uint32		TabWidgetHeight = 0;
		Uint32		TabTextAlign = ( UI_HALIGN_CENTER | UI_VALIGN_CENTER );
		Uint32		MinTabWidth = 32;
		Uint32		MaxTabWidth = 300;
		bool		TabsClosable = false;
		bool		SpecialBorderTabs = false; //! Indicates if the periferical tabs ( the left and right border tab ) are different from the central tabs.
		bool		DrawLineBelowTabs = false;
		ColorA		LineBelowTabsColor;
		Int32		LineBelowTabsYOffset = 0;
};

class ProgressBarStyleConfig : public FontStyleConfig {
	public:
		ProgressBarStyleConfig() {}

		ProgressBarStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		bool DisplayPercent = false;
		bool VerticalExpand = true;
		Vector2f MovementSpeed = Vector2f( 64.f, 0 );
		Rectf FillerPadding;
};

class WinMenuStyleConfig : public FontStyleConfig {
	public:
		WinMenuStyleConfig() {}

		WinMenuStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		Uint32				MarginBetweenButtons = 0;
		Uint32				ButtonMargin = 4;
		Uint32				MenuHeight = 0;
		Uint32				FirstButtonMargin = 1;
};

class DropDownListStyleConfig : public FontStyleConfig {
	public:
		DropDownListStyleConfig() {}

		DropDownListStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		Uint32 MaxNumVisibleItems = 10;
		bool PopUpToMainControl = false;
};

class WindowStyleConfig : public FontStyleConfig {
	public:
		WindowStyleConfig() {}

		WindowStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		Uint32		WinFlags = UI_WIN_DEFAULT_FLAGS;
		Sizei		DecorationSize;
		Sizei		BorderSize;
		Sizei		MinWindowSize;
		Vector2i	ButtonsPositionFixer;
		Uint32		ButtonsSeparation = 4;
		Int32		MinCornerDistance = 24;
		ColorA		TitleFontColor = ColorA( 255, 255, 255, 255 );
		Uint8		BaseAlpha = 255;
		bool		DecorationAutoSize = true;
		bool		BorderAutoSize = true;
};

class MenuStyleConfig : public FontStyleConfig {
	public:
		MenuStyleConfig() {}

		MenuStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		Recti				Padding = Recti(0, 0, 0, 0);
		Uint32				MinWidth = 0;
		Uint32				MinSpaceForIcons = 0;
		Uint32				MinRightMargin = 0;

};

class PushButtonStyleConfig : public FontStyleConfig {
	public:
		PushButtonStyleConfig() {}

		PushButtonStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		Int32				IconHorizontalMargin = 4;
		bool				IconAutoMargin = true;
		Sizei				IconMinSize;
};

class SliderStyleConfig {
	public:
		SliderStyleConfig() {}

		bool AllowHalfSliderOut = false;
		bool ExpandBackground = false;
};

class TooltipStyleConfig : public FontStyleConfig {
	public:
		TooltipStyleConfig() {}

		TooltipStyleConfig( FontStyleConfig fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig )
		{}

		Recti		Padding;
};

}}

#endif
