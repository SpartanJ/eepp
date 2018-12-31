#include <eepp/ui/uithemedefault.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uitabwidget.hpp>

namespace EE { namespace UI {

UIThemeDefault * UIThemeDefault::New( const std::string & name, const std::string & abbr, Font * defaultFont ) {
	return eeNew( UIThemeDefault, ( name, abbr, defaultFont ) );
}

UIThemeDefault::UIThemeDefault( const std::string& name, const std::string& Abbr, Graphics::Font * defaultFont ) :
	UITheme( name, Abbr, defaultFont )
{
	mFontStyleConfig.CharacterSize = 12;
	mFontStyleConfig.FontColor = Color( 230, 230, 230, 255 );
	mFontStyleConfig.FontOverColor = mFontStyleConfig.FontSelectedColor = Color( 255, 255, 255, 255 );
	mFontStyleConfig.ShadowColor = Color( 50, 50, 50, 150 );
	mFontStyleConfig.FontSelectionBackColor = Color( 150, 150, 150, 255 );
}

}}
