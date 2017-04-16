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

UITabWidgetStyleConfig UIThemeDefault::getTabWidgetStyleConfig() {
	UITabWidgetStyleConfig tabWidgetStyleConfig = UITheme::getTabWidgetStyleConfig();
	tabWidgetStyleConfig.TabSeparation = -1;
	tabWidgetStyleConfig.FontSelectedColor = Color( 255, 255, 255, 255 );
	tabWidgetStyleConfig.DrawLineBelowTabs = true;
	tabWidgetStyleConfig.LineBelowTabsColor = Color( 0, 0, 0, 255 );
	tabWidgetStyleConfig.LineBelowTabsYOffset = -1;
	return tabWidgetStyleConfig;
}

UIProgressBarStyleConfig UIThemeDefault::getProgressBarStyleConfig() {
	UIProgressBarStyleConfig progressBarStyleConfig = UITheme::getProgressBarStyleConfig();
	progressBarStyleConfig.DisplayPercent = true;
	progressBarStyleConfig.VerticalExpand = true;
	progressBarStyleConfig.FillerPadding = Rectf( 2, 2, 2, 2 );
	progressBarStyleConfig.MovementSpeed = Vector2f( 32, 0 );
	return progressBarStyleConfig;
}

UIWinMenuStyleConfig UIThemeDefault::getWinMenuStyleConfig() {
	UIWinMenuStyleConfig winMenuStyleConfig = UITheme::getWinMenuStyleConfig();
	winMenuStyleConfig.ButtonMargin = 12;
	return winMenuStyleConfig;
}

UIWindowStyleConfig UIThemeDefault::getWindowStyleConfig() {
	UIWindowStyleConfig windowStyleConfig = UITheme::getWindowStyleConfig();
	windowStyleConfig.WinFlags |= UI_WIN_DRAW_SHADOW;
	windowStyleConfig.ButtonsPositionFixer.x = -2;
	windowStyleConfig.TitleFontColor = Color( 230, 230, 230, 255 );
	return windowStyleConfig;
}

UIMenuStyleConfig UIThemeDefault::getMenuStyleConfig() {
	UIMenuStyleConfig menuStyleConfig = UITheme::getMenuStyleConfig();
	menuStyleConfig.MinWidth = 100;
	menuStyleConfig.MinSpaceForIcons = 24;
	menuStyleConfig.MinRightMargin = 8;
	menuStyleConfig.FontColor = Color( 230, 230, 230, 255 );
	menuStyleConfig.FontOverColor = Color( 255, 255, 255, 255 );
	return menuStyleConfig;
}

UISliderStyleConfig UIThemeDefault::getSliderStyleConfig() {
	UISliderStyleConfig sliderStyleConfig;
	sliderStyleConfig.AllowHalfSliderOut = true;
	return sliderStyleConfig;
}

UITooltipStyleConfig UIThemeDefault::getTooltipStyleConfig() {
	UITooltipStyleConfig tooltipStyleConfig = UITheme::getTooltipStyleConfig();
	tooltipStyleConfig.FontColor = Color( 0, 0, 0, 255 );
	tooltipStyleConfig.Padding = Rect( 4, 6, 4, 6 );
	return tooltipStyleConfig;
}

}}
