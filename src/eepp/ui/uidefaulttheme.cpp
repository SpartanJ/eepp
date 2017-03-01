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
	mFontStyleConfig.FontColor = ColorA( 230, 230, 230, 255 );
	mFontStyleConfig.FontOverColor = mFontStyleConfig.FontSelectedColor = ColorA( 255, 255, 255, 255 );
	mFontStyleConfig.FontShadowColor = ColorA( 50, 50, 50, 150 );
	mFontStyleConfig.FontSelectionBackColor = ColorA( 150, 150, 150, 255 );

	setTooltipFontColor( ColorA( 0, 0, 0, 255 ) );
	setTooltipPadding( Recti( 4, 6, 4, 6) );
}

TabWidgetStyleConfig UIDefaultTheme::getTabWidgetStyleConfig() {
	TabWidgetStyleConfig tabWidgetStyleConfig = UITheme::getTabWidgetStyleConfig();
	tabWidgetStyleConfig.TabSeparation = -1;
	tabWidgetStyleConfig.FontSelectedColor = ColorA( 255, 255, 255, 255 );
	tabWidgetStyleConfig.DrawLineBelowTabs = true;
	tabWidgetStyleConfig.LineBelowTabsColor = ColorA( 0, 0, 0, 255 );
	tabWidgetStyleConfig.LineBelowTabsYOffset = -1;
	return tabWidgetStyleConfig;
}

ProgressBarStyleConfig UIDefaultTheme::getProgressBarStyleConfig() {
	ProgressBarStyleConfig progressBarStyleConfig = UITheme::getProgressBarStyleConfig();
	progressBarStyleConfig.DisplayPercent = true;
	progressBarStyleConfig.VerticalExpand = true;
	progressBarStyleConfig.FillerPadding = Rectf( 2, 2, 2, 2 );
	progressBarStyleConfig.MovementSpeed = Vector2f( 32, 0 );
	return progressBarStyleConfig;
}

WinMenuStyleConfig UIDefaultTheme::getWinMenuStyleConfig() {
	WinMenuStyleConfig winMenuStyleConfig = UITheme::getWinMenuStyleConfig();
	winMenuStyleConfig.ButtonMargin = 12;
	return winMenuStyleConfig;
}

WindowStyleConfig UIDefaultTheme::getWindowStyleConfig() {
	WindowStyleConfig windowStyleConfig = UITheme::getWindowStyleConfig();
	windowStyleConfig.WinFlags |= UI_WIN_DRAW_SHADOW;
	windowStyleConfig.ButtonsPositionFixer.x = -2;
	windowStyleConfig.TitleFontColor = ColorA( 230, 230, 230, 255 );
	return windowStyleConfig;
}

MenuStyleConfig UIDefaultTheme::getMenuStyleConfig() {
	MenuStyleConfig menuStyleConfig = UITheme::getMenuStyleConfig();
	menuStyleConfig.MinWidth = 100;
	menuStyleConfig.MinSpaceForIcons = 24;
	menuStyleConfig.MinRightMargin = 8;
	menuStyleConfig.FontColor = ColorA( 230, 230, 230, 255 );
	menuStyleConfig.FontOverColor = ColorA( 255, 255, 255, 255 );
	return menuStyleConfig;
}

UICommonDialog * UIDefaultTheme::createCommonDialog( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 WinFlags, Sizei MinWindowSize, Uint8 BaseAlpha, Uint32 CDLFlags, std::string DefaultFilePattern, std::string DefaultDirectory ) {
	UICommonDialog::CreateParams DLGParams;
	DLGParams.setParent( Parent );
	DLGParams.setPosition( Pos );
	DLGParams.setSize( Size );
	DLGParams.Flags = Flags;
	DLGParams.windowStyleConfig.WinFlags = WinFlags;
	DLGParams.windowStyleConfig.MinWindowSize = MinWindowSize;
	DLGParams.windowStyleConfig.BaseAlpha = BaseAlpha;
	DLGParams.DefaultDirectory = DefaultDirectory;
	DLGParams.DefaultFilePattern = DefaultFilePattern;
	DLGParams.CDLFlags = CDLFlags;

	if ( getUseDefaultThemeValues() ) {
		DLGParams.Flags |= UI_DRAW_SHADOW;
		DLGParams.windowStyleConfig.WinFlags |= UI_WIN_DRAW_SHADOW;
		DLGParams.windowStyleConfig.ButtonsPositionFixer.x = -2;
		DLGParams.windowStyleConfig.TitleFontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UICommonDialog, ( DLGParams ) );
}

UIMessageBox * UIDefaultTheme::createMessageBox( UI_MSGBOX_TYPE Type, const String& Message, Uint32 WinFlags, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Sizei MinWindowSize, Uint8 BaseAlpha ) {
	UIMessageBox::CreateParams MsgBoxParams;
	MsgBoxParams.setParent( Parent );
	MsgBoxParams.setPosition( Pos );
	MsgBoxParams.setSize( Size );
	MsgBoxParams.Flags = Flags;
	MsgBoxParams.windowStyleConfig.WinFlags = WinFlags;
	MsgBoxParams.windowStyleConfig.MinWindowSize = MinWindowSize;
	MsgBoxParams.windowStyleConfig.BaseAlpha = BaseAlpha;
	MsgBoxParams.Type = Type;
	MsgBoxParams.Message = Message;

	if ( getUseDefaultThemeValues() ) {
		MsgBoxParams.Flags |= UI_DRAW_SHADOW;
		MsgBoxParams.windowStyleConfig.WinFlags |= UI_WIN_DRAW_SHADOW;
		MsgBoxParams.windowStyleConfig.ButtonsPositionFixer.x = -2;
		MsgBoxParams.windowStyleConfig.TitleFontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UIMessageBox, ( MsgBoxParams ) );
}

}}
