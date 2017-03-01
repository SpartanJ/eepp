#include <eepp/ui/uidefaulttheme.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uitabwidget.hpp>

namespace EE { namespace UI {

UIDefaultTheme::UIDefaultTheme( const std::string& name, const std::string& Abbr, Graphics::Font * defaultFont ) :
	UITheme( name, Abbr, defaultFont )
{
	mFontStyleConfig.fontColor = ColorA( 230, 230, 230, 255 );
	mFontStyleConfig.fontOverColor = mFontStyleConfig.fontSelectedColor = ColorA( 255, 255, 255, 255 );
	mFontStyleConfig.fontShadowColor = ColorA( 50, 50, 50, 150 );
	mFontStyleConfig.fontSelectionBackColor = ColorA( 150, 150, 150, 255 );

	setMenuFontColor( ColorA( 230, 230, 230, 255 ) );
	setMenuFontColorOver( ColorA( 255, 255, 255, 255 ) );
	setTooltipFontColor( ColorA( 0, 0, 0, 255 ) );
	setTooltipPadding( Recti( 4, 6, 4, 6) );
}

TabWidgetStyleConfig UIDefaultTheme::getTabWidgetStyleConfig() {
	TabWidgetStyleConfig tabWidgetStyleConfig = UITheme::getTabWidgetStyleConfig();
	tabWidgetStyleConfig.tabSeparation = -1;
	tabWidgetStyleConfig.fontSelectedColor = ColorA( 255, 255, 255, 255 );
	tabWidgetStyleConfig.drawLineBelowTabs = true;
	tabWidgetStyleConfig.lineBelowTabsColor = ColorA( 0, 0, 0, 255 );
	tabWidgetStyleConfig.lineBelowTabsYOffset = -1;
	return tabWidgetStyleConfig;
}

ProgressBarStyleConfig UIDefaultTheme::getProgressBarStyleConfig() {
	ProgressBarStyleConfig progressBarStyleConfig = UITheme::getProgressBarStyleConfig();
	progressBarStyleConfig.displayPercent = true;
	progressBarStyleConfig.verticalExpand = true;
	progressBarStyleConfig.fillerPadding = Rectf( 2, 2, 2, 2 );
	progressBarStyleConfig.movementSpeed = Vector2f( 32, 0 );
	return progressBarStyleConfig;
}

WinMenuStyleConfig UIDefaultTheme::getWinMenuStyleConfig() {
	WinMenuStyleConfig winMenuStyleConfig = UITheme::getWinMenuStyleConfig();
	winMenuStyleConfig.buttonMargin = 12;
	return winMenuStyleConfig;
}

WindowStyleConfig UIDefaultTheme::getWindowStyleConfig() {
	WindowStyleConfig windowStyleConfig = UITheme::getWindowStyleConfig();
	windowStyleConfig.winFlags |= UI_WIN_DRAW_SHADOW;
	windowStyleConfig.buttonsPositionFixer.x = -2;
	windowStyleConfig.titleFontColor = ColorA( 230, 230, 230, 255 );
	return windowStyleConfig;
}

UIPopUpMenu * UIDefaultTheme::createPopUpMenu( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Recti PaddingContainer, Uint32 MinWidth, Uint32 MinSpaceForIcons, Uint32 MinRightMargin ) {
	UIPopUpMenu::CreateParams MenuParams;
	MenuParams.setParent( Parent );
	MenuParams.setPosition( Pos );
	MenuParams.setSize( Size );
	MenuParams.Flags = Flags;
	MenuParams.PaddingContainer = PaddingContainer;
	MenuParams.MinWidth = MinWidth;
	MenuParams.MinSpaceForIcons = MinSpaceForIcons;
	MenuParams.MinRightMargin = MinRightMargin;

	if ( getUseDefaultThemeValues() ) {
		MenuParams.MinWidth = 100;
		MenuParams.MinSpaceForIcons = 24;
		MenuParams.MinRightMargin = 8;
		MenuParams.fontStyleConfig.fontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UIPopUpMenu, ( MenuParams ) );
}

UICommonDialog * UIDefaultTheme::createCommonDialog( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 WinFlags, Sizei MinWindowSize, Uint8 BaseAlpha, Uint32 CDLFlags, std::string DefaultFilePattern, std::string DefaultDirectory ) {
	UICommonDialog::CreateParams DLGParams;
	DLGParams.setParent( Parent );
	DLGParams.setPosition( Pos );
	DLGParams.setSize( Size );
	DLGParams.Flags = Flags;
	DLGParams.windowStyleConfig.winFlags = WinFlags;
	DLGParams.windowStyleConfig.minWindowSize = MinWindowSize;
	DLGParams.windowStyleConfig.baseAlpha = BaseAlpha;
	DLGParams.DefaultDirectory = DefaultDirectory;
	DLGParams.DefaultFilePattern = DefaultFilePattern;
	DLGParams.CDLFlags = CDLFlags;

	if ( getUseDefaultThemeValues() ) {
		DLGParams.Flags |= UI_DRAW_SHADOW;
		DLGParams.windowStyleConfig.winFlags |= UI_WIN_DRAW_SHADOW;
		DLGParams.windowStyleConfig.buttonsPositionFixer.x = -2;
		DLGParams.windowStyleConfig.titleFontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UICommonDialog, ( DLGParams ) );
}

UIMessageBox * UIDefaultTheme::createMessageBox( UI_MSGBOX_TYPE Type, const String& Message, Uint32 WinFlags, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Sizei MinWindowSize, Uint8 BaseAlpha ) {
	UIMessageBox::CreateParams MsgBoxParams;
	MsgBoxParams.setParent( Parent );
	MsgBoxParams.setPosition( Pos );
	MsgBoxParams.setSize( Size );
	MsgBoxParams.Flags = Flags;
	MsgBoxParams.windowStyleConfig.winFlags = WinFlags;
	MsgBoxParams.windowStyleConfig.minWindowSize = MinWindowSize;
	MsgBoxParams.windowStyleConfig.baseAlpha = BaseAlpha;
	MsgBoxParams.Type = Type;
	MsgBoxParams.Message = Message;

	if ( getUseDefaultThemeValues() ) {
		MsgBoxParams.Flags |= UI_DRAW_SHADOW;
		MsgBoxParams.windowStyleConfig.winFlags |= UI_WIN_DRAW_SHADOW;
		MsgBoxParams.windowStyleConfig.buttonsPositionFixer.x = -2;
		MsgBoxParams.windowStyleConfig.titleFontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UIMessageBox, ( MsgBoxParams ) );
}

}}
