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

		Uint32 getFontCharacterSize() const {
			return FontCharacterSize;
		}

		void setFontCharacterSize(const Uint32 & value) {
			FontCharacterSize = value;
		}

		Uint32 getFontStyle() const {
			return FontStyle;
		}

		void setFontStyle( const Uint32& style ) {
			FontStyle = style;
		}

		Float getOutlineThickness() const {
			return OutlineThickness;
		}

		void setOutlineThickness( const Uint32& outlineThickness ) {
			OutlineThickness = outlineThickness;
		}

		ColorA getOutlineColor() const {
			return OutlineColor;
		}

		void setOutlineColor(const ColorA & value) {
			OutlineColor = value;
		}

		FontStyleConfig() {}

		FontStyleConfig( const FontStyleConfig& fontStyleConfig ) :
			Font( fontStyleConfig.Font ),
			FontCharacterSize( fontStyleConfig.FontCharacterSize ),
			FontStyle( fontStyleConfig.FontStyle ),
			FontColor( fontStyleConfig.FontColor ),
			FontShadowColor( fontStyleConfig.FontShadowColor ),
			FontOverColor( fontStyleConfig.FontOverColor ),
			FontSelectedColor( fontStyleConfig.FontSelectedColor ),
			FontSelectionBackColor( fontStyleConfig.FontSelectionBackColor ),
			OutlineThickness( fontStyleConfig.OutlineThickness ),
			OutlineColor( fontStyleConfig.OutlineColor )
		{}

		void updateFontStyleConfig( const FontStyleConfig& fontStyleConfig ) {
			Font = fontStyleConfig.Font;
			FontStyle = fontStyleConfig.FontStyle;
			FontCharacterSize = fontStyleConfig.FontCharacterSize;
			FontColor = fontStyleConfig.FontColor;
			FontShadowColor = fontStyleConfig.FontShadowColor;
			FontOverColor = fontStyleConfig.FontOverColor;
			FontSelectedColor = fontStyleConfig.FontSelectedColor;
			FontSelectionBackColor = fontStyleConfig.FontSelectionBackColor;
			OutlineThickness = fontStyleConfig.OutlineThickness;
			OutlineColor = fontStyleConfig.OutlineColor;
		}

		Graphics::Font * Font = NULL;
		Uint32 FontCharacterSize = 12;
		Uint32 FontStyle = 0;
		ColorA FontColor = ColorA(255,255,255,255);
		ColorA FontShadowColor = ColorA(50,50,50,230);
		ColorA FontOverColor = ColorA(255,255,255,255);
		ColorA FontSelectedColor = ColorA(255,255,255,255);
		ColorA FontSelectionBackColor = ColorA(255,255,255,255);
		Float OutlineThickness = 0;
		ColorA OutlineColor = ColorA(0,0,0,255);
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
