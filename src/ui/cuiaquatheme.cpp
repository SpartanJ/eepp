#include "cuiaquatheme.hpp"
#include "cuiwindow.hpp"
#include "cuiwinmenu.hpp"
#include "cuipopupmenu.hpp"
#include "cuiprogressbar.hpp"
#include "cuicommondialog.hpp"

namespace EE { namespace UI {

cUIAquaTheme::cUIAquaTheme( const std::string& Name, const std::string& Abbr, cFont * DefaultFont ) :
	cUITheme( Name, Abbr, DefaultFont )
{
	FontSelectedColor( eeColorA( 255, 255, 255, 255 ) );
	FontShadowColor( eeColorA( 255, 255, 255, 150 ) );
}

cUIPopUpMenu * cUIAquaTheme::CreatePopUpMenu( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, Uint32 RowHeight, eeRecti PaddingContainer, Uint32 MinWidth, Uint32 MinSpaceForIcons, Uint32 MinRightMargin ) {
	cUIPopUpMenu::CreateParams MenuParams;
	MenuParams.Parent( Parent );
	MenuParams.PosSet( Pos );
	MenuParams.SizeSet( Size );
	MenuParams.Flags = Flags;
	MenuParams.RowHeight = RowHeight;
	MenuParams.PaddingContainer = PaddingContainer;
	MenuParams.MinWidth = MinWidth;
	MenuParams.MinSpaceForIcons = MinSpaceForIcons;
	MenuParams.MinRightMargin = MinRightMargin;

	if ( UseDefaultThemeValues() ) {
		MenuParams.MinWidth = 100;
		MenuParams.MinSpaceForIcons = 16;
		MenuParams.MinRightMargin = 8;
	}

	return eeNew( cUIPopUpMenu, ( MenuParams ) );
}

cUIProgressBar * cUIAquaTheme::CreateProgressBar( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, bool DisplayPercent, bool VerticalExpand, eeVector2f MovementSpeed, eeRectf FillerMargin ) {
	cUIProgressBar::CreateParams PBParams;
	PBParams.Parent( Parent );
	PBParams.PosSet( Pos );
	PBParams.SizeSet( Size );
	PBParams.Flags = Flags;
	PBParams.DisplayPercent = DisplayPercent;
	PBParams.VerticalExpand = VerticalExpand;
	PBParams.MovementSpeed = MovementSpeed;
	PBParams.FillerMargin = FillerMargin;

	if ( UseDefaultThemeValues() ) {
		PBParams.DisplayPercent = true;
	}

	cUIProgressBar * Ctrl = eeNew( cUIProgressBar, ( PBParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}
cUIWinMenu * cUIAquaTheme::CreateWinMenu( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, Uint32 MarginBetweenButtons, Uint32 ButtonMargin, Uint32 MenuHeight, Uint32 FirstButtonMargin ) {
	cUIWinMenu::CreateParams WinMenuParams;
	WinMenuParams.Parent( Parent );
	WinMenuParams.PosSet( Pos );
	WinMenuParams.SizeSet( Size );
	WinMenuParams.Flags = Flags;
	WinMenuParams.MarginBetweenButtons = MarginBetweenButtons;
	WinMenuParams.ButtonMargin = ButtonMargin;
	WinMenuParams.MenuHeight = MenuHeight;
	WinMenuParams.FirstButtonMargin = FirstButtonMargin;

	if ( UseDefaultThemeValues() ) {
		WinMenuParams.ButtonMargin = 12;
	}

	cUIWinMenu * Ctrl = eeNew( cUIWinMenu, ( WinMenuParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

cUIWindow * cUIAquaTheme::CreateWindow( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, Uint32 WinFlags, eeSize MinWindowSize, Uint8 BaseAlpha ) {
	cUIWindow::CreateParams WinParams;
	WinParams.Parent( Parent );
	WinParams.PosSet( Pos );
	WinParams.SizeSet( Size );
	WinParams.Flags = Flags;
	WinParams.WinFlags = WinFlags;
	WinParams.MinWindowSize = MinWindowSize;
	WinParams.BaseAlpha = BaseAlpha;

	if ( UseDefaultThemeValues() ) {
		if ( !( WinParams.Flags & UI_DRAW_SHADOW ) )
			WinParams.Flags |= UI_DRAW_SHADOW;

		if ( !( WinParams.WinFlags & UI_WIN_DRAW_SHADOW ) )
			WinParams.WinFlags |= UI_WIN_DRAW_SHADOW;

		WinParams.ButtonsPositionFixer.x = -2;
		WinParams.TitleFontColor = eeColorA( 0, 0, 0, 255 );
	}

	return eeNew( cUIWindow, ( WinParams ) );
}

cUICommonDialog * cUIAquaTheme::CreateCommonDialog( cUIControl * Parent, const eeSize& Size, const eeVector2i& Pos, const Uint32& Flags, Uint32 WinFlags, eeSize MinWindowSize, Uint8 BaseAlpha, Uint32 CDLFlags, std::string DefaultFilePattern, std::string DefaultDirectory ) {
	cUICommonDialog::CreateParams DLGParams;
	DLGParams.Parent( Parent );
	DLGParams.PosSet( Pos );
	DLGParams.SizeSet( Size );
	DLGParams.Flags = Flags;
	DLGParams.WinFlags = WinFlags;
	DLGParams.MinWindowSize = MinWindowSize;
	DLGParams.BaseAlpha = BaseAlpha;
	DLGParams.DefaultDirectory = DefaultDirectory;
	DLGParams.DefaultFilePattern = DefaultFilePattern;
	DLGParams.CDLFlags = CDLFlags;

	if ( UseDefaultThemeValues() ) {
		if ( !( DLGParams.Flags & UI_DRAW_SHADOW ) )
			DLGParams.Flags |= UI_DRAW_SHADOW;

		if ( !( DLGParams.WinFlags & UI_WIN_DRAW_SHADOW ) )
			DLGParams.WinFlags |= UI_WIN_DRAW_SHADOW;

		DLGParams.ButtonsPositionFixer.x = -2;
		DLGParams.TitleFontColor = eeColorA( 0, 0, 0, 255 );
	}

	return eeNew( cUICommonDialog, ( DLGParams ) );
}

}}
