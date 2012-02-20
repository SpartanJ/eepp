#ifndef EE_UICUIAQUATHEME_HPP
#define EE_UICUIAQUATHEME_HPP

#include "cuitheme.hpp"

namespace EE { namespace UI {

class EE_API cUIAquaTheme : public cUITheme {
	public:
		cUIAquaTheme( const std::string& Name, const std::string& Abbr, cFont * DefaultFont = NULL );

		virtual cUIPopUpMenu * CreatePopUpMenu( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE | UI_AUTO_PADDING, Uint32 RowHeight = 0, eeRecti PaddingContainer = eeRecti(), Uint32 MinWidth = 100, Uint32 MinSpaceForIcons = 16, Uint32 MinRightMargin = 8 );

		virtual cUIProgressBar * CreateProgressBar( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS, bool DisplayPercent = true, bool VerticalExpand = false, eeVector2f MovementSpeed = eeVector2f( -64, 0 ), eeRectf FillerMargin = eeRectf() );

		virtual cUIWinMenu * CreateWinMenu( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS, Uint32 MarginBetweenButtons = 0, Uint32 ButtonMargin = 12, Uint32 MenuHeight = 0, Uint32 FirstButtonMargin = 1 );

		virtual cUIWindow * CreateWindow( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED, Uint32 WinFlags = UI_WIN_DEFAULT_FLAGS, eeSize MinWindowSize = eeSize(0,0), Uint8 BaseAlpha = 255 );

		virtual cUICommonDialog * CreateCommonDialog( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED, Uint32 WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON, eeSize MinWindowSize = eeSize(0,0), Uint8 BaseAlpha = 255, Uint32 CDLFlags = UI_CDL_DEFAULT_FLAGS, std::string DefaultFilePattern = "*", std::string DefaultDirectory = GetProcessPath() );

		virtual cUIComboBox * CreateComboBox( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, Uint32 MinNumVisibleItems = 6, bool PopUpToMainControl = false, cUIListBox * ListBox = NULL );

		virtual cUIDropDownList * CreateDropDownList( cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE, Uint32 MinNumVisibleItems = 6, bool PopUpToMainControl = false, cUIListBox * ListBox = NULL );

		virtual cUIMessageBox * CreateMessageBox( UI_MSGBOX_TYPE Type = MSGBOX_OKCANCEL, const String& Message = String(), Uint32 WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, cUIControl * Parent = NULL, const eeSize& Size = eeSize(), const eeVector2i& Pos = eeVector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED, eeSize MinWindowSize = eeSize(0,0), Uint8 BaseAlpha = 255 );
};

}}

#endif
