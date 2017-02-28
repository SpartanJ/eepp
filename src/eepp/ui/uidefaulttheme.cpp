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
	setTabSeparation( -1 );
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

UIWinMenu * UIDefaultTheme::createWinMenu( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 MarginBetweenButtons, Uint32 ButtonMargin, Uint32 MenuHeight, Uint32 FirstButtonMargin ) {
	UIWinMenu::CreateParams WinMenuParams;
	WinMenuParams.setParent( Parent );
	WinMenuParams.setPosition( Pos );
	WinMenuParams.setSize( Size );
	WinMenuParams.Flags = Flags;
	WinMenuParams.marginBetweenButtons = MarginBetweenButtons;
	WinMenuParams.buttonMargin = ButtonMargin;
	WinMenuParams.menuHeight = MenuHeight;
	WinMenuParams.firstButtonMargin = FirstButtonMargin;

	if ( getUseDefaultThemeValues() ) {
		WinMenuParams.buttonMargin = 12;
	}

	UIWinMenu * Ctrl = eeNew( UIWinMenu, ( WinMenuParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

UIWindow * UIDefaultTheme::createWindow( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 WinFlags, Sizei MinWindowSize, Uint8 BaseAlpha ) {
	UIWindow::CreateParams WinParams;
	WinParams.setParent( Parent );
	WinParams.setPosition( Pos );
	WinParams.setSize( Size );
	WinParams.Flags = Flags;
	WinParams.WinFlags = WinFlags;
	WinParams.MinWindowSize = MinWindowSize;
	WinParams.BaseAlpha = BaseAlpha;

	if ( getUseDefaultThemeValues() ) {
		WinParams.Flags |= UI_DRAW_SHADOW;
		WinParams.WinFlags |= UI_WIN_DRAW_SHADOW;
		WinParams.ButtonsPositionFixer.x = -2;
		WinParams.TitleFontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UIWindow, ( WinParams ) );
}

UICommonDialog * UIDefaultTheme::createCommonDialog( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 WinFlags, Sizei MinWindowSize, Uint8 BaseAlpha, Uint32 CDLFlags, std::string DefaultFilePattern, std::string DefaultDirectory ) {
	UICommonDialog::CreateParams DLGParams;
	DLGParams.setParent( Parent );
	DLGParams.setPosition( Pos );
	DLGParams.setSize( Size );
	DLGParams.Flags = Flags;
	DLGParams.WinFlags = WinFlags;
	DLGParams.MinWindowSize = MinWindowSize;
	DLGParams.BaseAlpha = BaseAlpha;
	DLGParams.DefaultDirectory = DefaultDirectory;
	DLGParams.DefaultFilePattern = DefaultFilePattern;
	DLGParams.CDLFlags = CDLFlags;

	if ( getUseDefaultThemeValues() ) {
		DLGParams.Flags |= UI_DRAW_SHADOW;
		DLGParams.WinFlags |= UI_WIN_DRAW_SHADOW;
		DLGParams.ButtonsPositionFixer.x = -2;
		DLGParams.TitleFontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UICommonDialog, ( DLGParams ) );
}

UIMessageBox * UIDefaultTheme::createMessageBox( UI_MSGBOX_TYPE Type, const String& Message, Uint32 WinFlags, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Sizei MinWindowSize, Uint8 BaseAlpha ) {
	UIMessageBox::CreateParams MsgBoxParams;
	MsgBoxParams.setParent( Parent );
	MsgBoxParams.setPosition( Pos );
	MsgBoxParams.setSize( Size );
	MsgBoxParams.Flags = Flags;
	MsgBoxParams.WinFlags = WinFlags;
	MsgBoxParams.MinWindowSize = MinWindowSize;
	MsgBoxParams.BaseAlpha = BaseAlpha;
	MsgBoxParams.Type = Type;
	MsgBoxParams.Message = Message;

	if ( getUseDefaultThemeValues() ) {
		MsgBoxParams.Flags |= UI_DRAW_SHADOW;
		MsgBoxParams.WinFlags |= UI_WIN_DRAW_SHADOW;
		MsgBoxParams.ButtonsPositionFixer.x = -2;
		MsgBoxParams.TitleFontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UIMessageBox, ( MsgBoxParams ) );
}

UIDropDownList * UIDefaultTheme::createDropDownList( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 MinNumVisibleItems, bool PopUpToMainControl, UIListBox * ListBox ) {
	UIDropDownList::CreateParams DDLParams;
	DDLParams.setParent( Parent );
	DDLParams.setPosition( Pos );
	DDLParams.setSize( Size );
	DDLParams.Flags = Flags;
	DDLParams.MinNumVisibleItems = MinNumVisibleItems;
	DDLParams.PopUpToMainControl = PopUpToMainControl;
	DDLParams.ListBox = ListBox;

	if ( getUseDefaultThemeValues() ) {
		DDLParams.Flags |= UI_AUTO_SIZE;
	}

	UIDropDownList * Ctrl = eeNew( UIDropDownList, ( DDLParams ) );
	Ctrl->setVisible( true );
	Ctrl->setEnabled( true );
	return Ctrl;
}

}}
