#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uicontrol.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

SINGLETON_DECLARE_IMPLEMENTATION(UIThemeManager)

UIThemeManager::UIThemeManager() :
	ResourceManager<UITheme>( true ),
	mFont( NULL ),
	mThemeDefault( NULL ),
	mautoApplyDefaultTheme( true ),
	mEnableDefaultEffects( false ),
	mFadeInTime( Milliseconds( 100.f ) ),
	mFadeOutTime( Milliseconds ( 100.f ) ),
	mtooltipTimeToShow( Milliseconds( 200 ) ),
	mtooltipFollowMouse( true ),
	mcursorSize( 16, 16 )
{
}

UIThemeManager::~UIThemeManager() {
}

void UIThemeManager::defaultFont( Font * Font ) {
	mFont = Font;
}

Font * UIThemeManager::defaultFont() const {
	return mFont;
}

void UIThemeManager::setTheme( const std::string& Theme ) {
	setTheme( getByName( Theme ) );
}

void UIThemeManager::setTheme( UITheme * Theme ) {
	UIControl * MainCtrl = UIManager::instance()->mainControl();

	if ( NULL != MainCtrl ) {
		MainCtrl->setThemeToChilds( Theme );
		MainCtrl->setTheme( Theme );
	}
}

void UIThemeManager::defaultTheme( UITheme * Theme ) {
	mThemeDefault = Theme;
}

void UIThemeManager::defaultTheme( const std::string& Theme ) {
	defaultTheme( UIThemeManager::instance()->getByName( Theme ) );
}

UITheme * UIThemeManager::defaultTheme() const {
	return mThemeDefault;
}

void UIThemeManager::applyDefaultTheme( UIControl * Control ) {
	if ( mautoApplyDefaultTheme && NULL != mThemeDefault && NULL != Control )
		Control->setTheme( mThemeDefault );
}

void UIThemeManager::autoApplyDefaultTheme( const bool& apply ) {
	mautoApplyDefaultTheme = apply;
}

const bool& UIThemeManager::autoApplyDefaultTheme() const {
	return mautoApplyDefaultTheme;
}

void UIThemeManager::defaultEffectsEnabled( const bool& Enabled ) {
	mEnableDefaultEffects = Enabled;
}

const bool& UIThemeManager::defaultEffectsEnabled() const {
	return mEnableDefaultEffects;
}

const Time& UIThemeManager::controlsFadeInTime() const {
	return mFadeInTime;
}

void UIThemeManager::controlsFadeInTime( const Time& Time ) {
	mFadeInTime = Time;
}

const Time& UIThemeManager::controlsFadeOutTime() const {
	return mFadeOutTime;
}

void UIThemeManager::controlsFadeOutTime( const Time& Time ) {
	mFadeOutTime = Time;
}

void UIThemeManager::tooltipTimeToShow( const Time& Time ) {
	mtooltipTimeToShow = Time;
}

const Time& UIThemeManager::tooltipTimeToShow() const {
	return mtooltipTimeToShow;
}

void UIThemeManager::tooltipFollowMouse( const bool& Follow ) {
	mtooltipFollowMouse = Follow;
}

const bool& UIThemeManager::tooltipFollowMouse() const {
	return mtooltipFollowMouse;
}

void UIThemeManager::cursorSize( const Sizei& Size ) {
	mcursorSize = Size;
}

const Sizei& UIThemeManager::cursorSize() const {
	return mcursorSize;
}

}}
