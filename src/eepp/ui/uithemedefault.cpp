#include <eepp/ui/uithemedefault.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uicommondialog.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uitabwidget.hpp>

namespace EE { namespace UI {

UIThemeDefault::UIThemeDefault( const std::string& name, const std::string& Abbr, Graphics::Font * defaultFont ) :
	UITheme( name, Abbr, defaultFont )
{
	mFontStyleConfig.FontCharacterSize = 12;
	mFontStyleConfig.FontColor = ColorA( 230, 230, 230, 255 );
	mFontStyleConfig.FontOverColor = mFontStyleConfig.FontSelectedColor = ColorA( 255, 255, 255, 255 );
	mFontStyleConfig.FontShadowColor = ColorA( 50, 50, 50, 150 );
	mFontStyleConfig.FontSelectionBackColor = ColorA( 150, 150, 150, 255 );
}

TabWidgetStyleConfig UIThemeDefault::getTabWidgetStyleConfig() {
	TabWidgetStyleConfig tabWidgetStyleConfig = UITheme::getTabWidgetStyleConfig();
	tabWidgetStyleConfig.TabSeparation = -1;
	tabWidgetStyleConfig.FontSelectedColor = ColorA( 255, 255, 255, 255 );
	tabWidgetStyleConfig.DrawLineBelowTabs = true;
	tabWidgetStyleConfig.LineBelowTabsColor = ColorA( 0, 0, 0, 255 );
	tabWidgetStyleConfig.LineBelowTabsYOffset = -1;
	return tabWidgetStyleConfig;
}

ProgressBarStyleConfig UIThemeDefault::getProgressBarStyleConfig() {
	ProgressBarStyleConfig progressBarStyleConfig = UITheme::getProgressBarStyleConfig();
	progressBarStyleConfig.DisplayPercent = true;
	progressBarStyleConfig.VerticalExpand = true;
	progressBarStyleConfig.FillerPadding = Rectf( 2, 2, 2, 2 );
	progressBarStyleConfig.MovementSpeed = Vector2f( 32, 0 );
	return progressBarStyleConfig;
}

WinMenuStyleConfig UIThemeDefault::getWinMenuStyleConfig() {
	WinMenuStyleConfig winMenuStyleConfig = UITheme::getWinMenuStyleConfig();
	winMenuStyleConfig.ButtonMargin = 12;
	return winMenuStyleConfig;
}

WindowStyleConfig UIThemeDefault::getWindowStyleConfig() {
	WindowStyleConfig windowStyleConfig = UITheme::getWindowStyleConfig();
	windowStyleConfig.WinFlags |= UI_WIN_DRAW_SHADOW;
	windowStyleConfig.ButtonsPositionFixer.x = -2;
	windowStyleConfig.TitleFontColor = ColorA( 230, 230, 230, 255 );
	return windowStyleConfig;
}

MenuStyleConfig UIThemeDefault::getMenuStyleConfig() {
	MenuStyleConfig menuStyleConfig = UITheme::getMenuStyleConfig();
	menuStyleConfig.MinWidth = 100;
	menuStyleConfig.MinSpaceForIcons = 24;
	menuStyleConfig.MinRightMargin = 8;
	menuStyleConfig.FontColor = ColorA( 230, 230, 230, 255 );
	menuStyleConfig.FontOverColor = ColorA( 255, 255, 255, 255 );
	return menuStyleConfig;
}

SliderStyleConfig UIThemeDefault::getSliderStyleConfig() {
	SliderStyleConfig sliderStyleConfig;
	sliderStyleConfig.AllowHalfSliderOut = true;
	return sliderStyleConfig;
}

TooltipStyleConfig UIThemeDefault::getTooltipStyleConfig() {
	TooltipStyleConfig tooltipStyleConfig = UITheme::getTooltipStyleConfig();
	tooltipStyleConfig.FontColor = ColorA( 0, 0, 0, 255 );
	tooltipStyleConfig.Padding = Recti( 4, 6, 4, 6 );
	return tooltipStyleConfig;
}

}}
