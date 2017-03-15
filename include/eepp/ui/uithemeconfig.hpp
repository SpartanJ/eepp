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
		const ColorA& getFontOverColor() const {
			return FontOverColor;
		}

		const ColorA& getFontSelectedColor() const {
			return FontSelectedColor;
		}

		ColorA getFontSelectionBackColor() const {
			return FontSelectionBackColor;
		}

		void setFontShadowColor( const ColorA& color ) {
			ShadowColor = color;
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

		UIFontStyleConfig() : FontStyleConfig() {}

		UIFontStyleConfig( const UIFontStyleConfig& fontStyleConfig ) :
			FontStyleConfig( fontStyleConfig ),
			FontOverColor( fontStyleConfig.FontOverColor ),
			FontSelectedColor( fontStyleConfig.FontSelectedColor ),
			FontSelectionBackColor( fontStyleConfig.FontSelectionBackColor )
		{}

		virtual void updateFontStyleConfig( const UIFontStyleConfig& fontStyleConfig ) {
			FontStyleConfig::updateFontStyleConfig( fontStyleConfig );
			FontOverColor = fontStyleConfig.FontOverColor;
			FontSelectedColor = fontStyleConfig.FontSelectedColor;
			FontSelectionBackColor = fontStyleConfig.FontSelectionBackColor;
		}

		ColorA FontOverColor = ColorA(255,255,255,255);
		ColorA FontSelectedColor = ColorA(255,255,255,255);
		ColorA FontSelectionBackColor = ColorA(255,255,255,255);
};

class UITabWidgetStyleConfig : public UIFontStyleConfig {
	public:
		UITabWidgetStyleConfig() {}

		UITabWidgetStyleConfig( UIFontStyleConfig fontStyleConfig ) :
			UIFontStyleConfig( fontStyleConfig )
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

class UIProgressBarStyleConfig : public UIFontStyleConfig {
	public:
		UIProgressBarStyleConfig() {}

		UIProgressBarStyleConfig( UIFontStyleConfig fontStyleConfig ) :
			UIFontStyleConfig( fontStyleConfig )
		{}

		bool DisplayPercent = false;
		bool VerticalExpand = true;
		Vector2f MovementSpeed = Vector2f( 64.f, 0 );
		Rectf FillerPadding;
};

class UIWinMenuStyleConfig : public UIFontStyleConfig {
	public:
		UIWinMenuStyleConfig() {}

		UIWinMenuStyleConfig( UIFontStyleConfig fontStyleConfig ) :
			UIFontStyleConfig( fontStyleConfig )
		{}

		Uint32				MarginBetweenButtons = 0;
		Uint32				ButtonMargin = 4;
		Uint32				MenuHeight = 0;
		Uint32				FirstButtonMargin = 1;
};

class UIDropDownListStyleConfig : public UIFontStyleConfig {
	public:
		UIDropDownListStyleConfig() {}

		UIDropDownListStyleConfig( UIFontStyleConfig fontStyleConfig ) :
			UIFontStyleConfig( fontStyleConfig )
		{}

		Uint32 MaxNumVisibleItems = 10;
		bool PopUpToMainControl = false;
};

class UIWindowStyleConfig : public UIFontStyleConfig {
	public:
		UIWindowStyleConfig() {}

		UIWindowStyleConfig( UIFontStyleConfig fontStyleConfig ) :
			UIFontStyleConfig( fontStyleConfig )
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

class UIMenuStyleConfig : public UIFontStyleConfig {
	public:
		UIMenuStyleConfig() {}

		UIMenuStyleConfig( UIFontStyleConfig fontStyleConfig ) :
			UIFontStyleConfig( fontStyleConfig )
		{}

		Recti				Padding = Recti(0, 0, 0, 0);
		Uint32				MinWidth = 0;
		Uint32				MinSpaceForIcons = 0;
		Uint32				MinRightMargin = 0;

};

class UIPushButtonStyleConfig : public UIFontStyleConfig {
	public:
		UIPushButtonStyleConfig() {}

		UIPushButtonStyleConfig( UIFontStyleConfig fontStyleConfig ) :
			UIFontStyleConfig( fontStyleConfig )
		{}

		Int32				IconHorizontalMargin = 4;
		bool				IconAutoMargin = true;
		Sizei				IconMinSize;
};

class UISliderStyleConfig {
	public:
		UISliderStyleConfig() {}

		bool AllowHalfSliderOut = false;
		bool ExpandBackground = false;
};

class UITooltipStyleConfig : public UIFontStyleConfig {
	public:
		UITooltipStyleConfig() {}

		UITooltipStyleConfig( UIFontStyleConfig fontStyleConfig ) :
			UIFontStyleConfig( fontStyleConfig )
		{}

		Recti		Padding;
};

}}

#endif
