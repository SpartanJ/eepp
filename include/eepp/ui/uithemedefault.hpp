#ifndef EE_UICUIDEFAULTTHEME_HPP
#define EE_UICUIDEFAULTTHEME_HPP

#include <eepp/ui/uitheme.hpp>

namespace EE { namespace UI {

class EE_API UIThemeDefault : public UITheme {
	public:
		static UIThemeDefault * New( const std::string& name, const std::string& abbr, Graphics::Font * defaultFont = NULL );

		UIThemeDefault( const std::string& name, const std::string& abbr, Graphics::Font * defaultFont = NULL );

		UITabWidgetStyleConfig getTabWidgetStyleConfig();

		UIProgressBarStyleConfig getProgressBarStyleConfig();

		UIWinMenuStyleConfig getWinMenuStyleConfig();

		UIWindowStyleConfig getWindowStyleConfig();

		UIMenuStyleConfig getMenuStyleConfig();

		UISliderStyleConfig getSliderStyleConfig();

		UITooltipStyleConfig getTooltipStyleConfig();
};

}}

#endif
