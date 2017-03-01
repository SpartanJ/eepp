#ifndef EE_UICUIDEFAULTTHEME_HPP
#define EE_UICUIDEFAULTTHEME_HPP

#include <eepp/ui/uitheme.hpp>

namespace EE { namespace UI {

class EE_API UIDefaultTheme : public UITheme {
	public:
		UIDefaultTheme( const std::string& name, const std::string& abbr, Graphics::Font * defaultFont = NULL );

		TabWidgetStyleConfig getTabWidgetStyleConfig();

		ProgressBarStyleConfig getProgressBarStyleConfig();

		WinMenuStyleConfig getWinMenuStyleConfig();

		WindowStyleConfig getWindowStyleConfig();

		virtual UIPopUpMenu * createPopUpMenu(UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS | UI_AUTO_SIZE | UI_AUTO_PADDING, Recti PaddingContainer = Recti(), Uint32 MinWidth = 100, Uint32 MinSpaceForIcons = 16, Uint32 MinRightMargin = 8 );

		virtual UICommonDialog * createCommonDialog( UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED, Uint32 WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON, Sizei MinWindowSize = Sizei(0,0), Uint8 BaseAlpha = 255, Uint32 CDLFlags = UI_CDL_DEFAULT_FLAGS, std::string DefaultFilePattern = "*", std::string DefaultDirectory = Sys::getProcessPath() );

		virtual UIMessageBox * createMessageBox( UI_MSGBOX_TYPE Type = MSGBOX_OKCANCEL, const String& Message = String(), Uint32 WinFlags = UI_WIN_DEFAULT_FLAGS | UI_WIN_MODAL, UIControl * Parent = NULL, const Sizei& Size = Sizei(), const Vector2i& Pos = Vector2i(), const Uint32& Flags = UI_CONTROL_DEFAULT_FLAGS_CENTERED, Sizei MinWindowSize = Sizei(0,0), Uint8 BaseAlpha = 255 );
};

}}

#endif
