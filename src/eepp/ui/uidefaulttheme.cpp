#include <eepp/ui/uidefaulttheme.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uitabwidget.hpp>

namespace EE { namespace UI {

UIDefaultTheme::UIDefaultTheme( const std::string& Name, const std::string& Abbr, Graphics::Font * DefaultFont ) :
	UITheme( Name, Abbr, DefaultFont )
{
	FontColor( ColorA( 230, 230, 230, 255 ) );
	FontOverColor( ColorA( 255, 255, 255, 255 ) );
	FontSelectedColor( ColorA( 255, 255, 255, 255 ) );
	FontShadowColor( ColorA( 50, 50, 50, 150 ) );
}

UIPopUpMenu * UIDefaultTheme::CreatePopUpMenu( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 RowHeight, Recti PaddingContainer, Uint32 MinWidth, Uint32 MinSpaceForIcons, Uint32 MinRightMargin ) {
	UIPopUpMenu::CreateParams MenuParams;
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
		MenuParams.MinSpaceForIcons = 24;
		MenuParams.MinRightMargin = 8;
		MenuParams.FontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UIPopUpMenu, ( MenuParams ) );
}

UIProgressBar * UIDefaultTheme::CreateProgressBar( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, bool DisplayPercent, bool VerticalExpand, Vector2f MovementSpeed, Rectf FillerMargin ) {
	UIProgressBar::CreateParams PBParams;
	PBParams.Parent( Parent );
	PBParams.PosSet( Pos );
	PBParams.SizeSet( Size );
	PBParams.Flags = Flags;
	PBParams.DisplayPercent = DisplayPercent;
	PBParams.VerticalExpand = VerticalExpand;
	PBParams.MovementSpeed = MovementSpeed;
	PBParams.FillerMargin = FillerMargin;

	if ( UseDefaultThemeValues() ) {
		PBParams.Flags |= UI_AUTO_SIZE;
		PBParams.DisplayPercent = true;
		PBParams.VerticalExpand = true;
		PBParams.FillerMargin = Rectf( 2, 2, 2, 2 );
		PBParams.MovementSpeed = Vector2f( 32, 0 );
	}

	UIProgressBar * Ctrl = eeNew( UIProgressBar, ( PBParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

UIWinMenu * UIDefaultTheme::CreateWinMenu( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 MarginBetweenButtons, Uint32 ButtonMargin, Uint32 MenuHeight, Uint32 FirstButtonMargin ) {
	UIWinMenu::CreateParams WinMenuParams;
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

	UIWinMenu * Ctrl = eeNew( UIWinMenu, ( WinMenuParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

UIWindow * UIDefaultTheme::CreateWindow( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 WinFlags, Sizei MinWindowSize, Uint8 BaseAlpha ) {
	UIWindow::CreateParams WinParams;
	WinParams.Parent( Parent );
	WinParams.PosSet( Pos );
	WinParams.SizeSet( Size );
	WinParams.Flags = Flags;
	WinParams.WinFlags = WinFlags;
	WinParams.MinWindowSize = MinWindowSize;
	WinParams.BaseAlpha = BaseAlpha;

	if ( UseDefaultThemeValues() ) {
		WinParams.Flags |= UI_DRAW_SHADOW;
		WinParams.WinFlags |= UI_WIN_DRAW_SHADOW;
		WinParams.ButtonsPositionFixer.x = -2;
		WinParams.TitleFontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UIWindow, ( WinParams ) );
}

UICommonDialog * UIDefaultTheme::CreateCommonDialog( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 WinFlags, Sizei MinWindowSize, Uint8 BaseAlpha, Uint32 CDLFlags, std::string DefaultFilePattern, std::string DefaultDirectory ) {
	UICommonDialog::CreateParams DLGParams;
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
		DLGParams.Flags |= UI_DRAW_SHADOW;
		DLGParams.WinFlags |= UI_WIN_DRAW_SHADOW;
		DLGParams.ButtonsPositionFixer.x = -2;
		DLGParams.TitleFontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UICommonDialog, ( DLGParams ) );
}

UIMessageBox * UIDefaultTheme::CreateMessageBox( UI_MSGBOX_TYPE Type, const String& Message, Uint32 WinFlags, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Sizei MinWindowSize, Uint8 BaseAlpha ) {
	UIMessageBox::CreateParams MsgBoxParams;
	MsgBoxParams.Parent( Parent );
	MsgBoxParams.PosSet( Pos );
	MsgBoxParams.SizeSet( Size );
	MsgBoxParams.Flags = Flags;
	MsgBoxParams.WinFlags = WinFlags;
	MsgBoxParams.MinWindowSize = MinWindowSize;
	MsgBoxParams.BaseAlpha = BaseAlpha;
	MsgBoxParams.Type = Type;
	MsgBoxParams.Message = Message;

	if ( UseDefaultThemeValues() ) {
		MsgBoxParams.Flags |= UI_DRAW_SHADOW;
		MsgBoxParams.WinFlags |= UI_WIN_DRAW_SHADOW;
		MsgBoxParams.ButtonsPositionFixer.x = -2;
		MsgBoxParams.TitleFontColor = ColorA( 230, 230, 230, 255 );
	}

	return eeNew( UIMessageBox, ( MsgBoxParams ) );
}

UIComboBox * UIDefaultTheme::CreateComboBox( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 MinNumVisibleItems, bool PopUpToMainControl, UIListBox * ListBox ) {
	UIComboBox::CreateParams ComboParams;
	ComboParams.Parent( Parent );
	ComboParams.PosSet( Pos );
	ComboParams.SizeSet( Size );
	ComboParams.Flags = Flags;
	ComboParams.MinNumVisibleItems = MinNumVisibleItems;
	ComboParams.PopUpToMainControl = PopUpToMainControl;
	ComboParams.ListBox = ListBox;

	if ( UseDefaultThemeValues() ) {
		ComboParams.Flags |= UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED;
	}

	UIComboBox * Ctrl = eeNew( UIComboBox, ( ComboParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

UIDropDownList * UIDefaultTheme::CreateDropDownList( UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags, Uint32 MinNumVisibleItems, bool PopUpToMainControl, UIListBox * ListBox ) {
	UIDropDownList::CreateParams DDLParams;
	DDLParams.Parent( Parent );
	DDLParams.PosSet( Pos );
	DDLParams.SizeSet( Size );
	DDLParams.Flags = Flags;
	DDLParams.MinNumVisibleItems = MinNumVisibleItems;
	DDLParams.PopUpToMainControl = PopUpToMainControl;
	DDLParams.ListBox = ListBox;

	if ( UseDefaultThemeValues() ) {
		DDLParams.Flags |= UI_AUTO_SIZE;
	}

	UIDropDownList * Ctrl = eeNew( UIDropDownList, ( DDLParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

UITabWidget * UIDefaultTheme::CreateTabWidget( UIControl *Parent, const Sizei &Size, const Vector2i &Pos, const Uint32 &Flags, const bool &TabsClosable, const bool &SpecialBorderTabs, const Int32 &TabSeparation, const Uint32 &MaxTextLength, const Uint32 &TabWidgetHeight, const Uint32 &TabTextAlign, const Uint32 &MinTabWidth, const Uint32 &MaxTabWidth ) {
	UITabWidget::CreateParams TabWidgetParams;
	TabWidgetParams.Parent( Parent );
	TabWidgetParams.PosSet( Pos );
	TabWidgetParams.SizeSet( Size );
	TabWidgetParams.Flags = Flags;
	TabWidgetParams.TabsClosable = TabsClosable;
	TabWidgetParams.SpecialBorderTabs = SpecialBorderTabs;
	TabWidgetParams.TabSeparation = TabSeparation;
	TabWidgetParams.MaxTextLength = MaxTextLength;
	TabWidgetParams.TabWidgetHeight = TabWidgetHeight;
	TabWidgetParams.TabTextAlign = TabTextAlign;
	TabWidgetParams.MinTabWidth = MinTabWidth;
	TabWidgetParams.MaxTabWidth = MaxTabWidth;

	if ( UseDefaultThemeValues() ) {
		TabWidgetParams.TabSeparation = -1;
		TabWidgetParams.FontSelectedColor = ColorA( 255, 255, 255, 255 );
		TabWidgetParams.DrawLineBelowTabs = true;
		TabWidgetParams.LineBelowTabsColor = ColorA( 0, 0, 0, 255 );
		TabWidgetParams.LineBewowTabsYOffset = -1;
	}

	UITabWidget * Ctrl = eeNew( UITabWidget, ( TabWidgetParams ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

UITooltip * UIDefaultTheme::CreateTooltip( UIControl * TooltipOf, UIControl * Parent, const Sizei& Size, const Vector2i& Pos, const Uint32& Flags ) {
	UITooltip::CreateParams TooltipParams;
	TooltipParams.Parent( Parent );
	TooltipParams.PosSet( Pos );
	TooltipParams.SizeSet( Size );
	TooltipParams.Flags = Flags;

	if ( UseDefaultThemeValues() ) {
		TooltipParams.Flags &= ~UI_AUTO_PADDING;
		TooltipParams.FontColor = ColorA( 0, 0, 0, 255 );
		TooltipParams.Padding = Recti( 4, 6, 4, 6 );
	}

	UITooltip * Ctrl = eeNew( UITooltip, ( TooltipParams, TooltipOf ) );
	Ctrl->Visible( true );
	Ctrl->Enabled( true );
	return Ctrl;
}

}}
