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
}

}}
