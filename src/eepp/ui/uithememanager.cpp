#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uicontrol.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

SINGLETON_DECLARE_IMPLEMENTATION(UIThemeManager)

UIThemeManager::UIThemeManager() :
	ResourceManager<UITheme>( true ),
	mFont( NULL ),
	mThemeDefault( NULL ),
	mAutoApplyDefaultTheme( true ),
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

void UIThemeManager::setDefaultFont( Font * Font ) {
	mFont = Font;
}

Font * UIThemeManager::getDefaultFont() const {
	return mFont;
}

void UIThemeManager::setTheme( const std::string& Theme ) {
	setTheme( getByName( Theme ) );
}

void UIThemeManager::setTheme( UITheme * Theme ) {
	UIControl * MainCtrl = UIManager::instance()->getMainControl();

	if ( NULL != MainCtrl ) {
		MainCtrl->setThemeToChilds( Theme );
		MainCtrl->setTheme( Theme );
	}
}

void UIThemeManager::setDefaultTheme( UITheme * Theme ) {
	mThemeDefault = Theme;
}

void UIThemeManager::setDefaultTheme( const std::string& Theme ) {
	setDefaultTheme( UIThemeManager::instance()->getByName( Theme ) );
}

UITheme * UIThemeManager::getDefaultTheme() const {
	return mThemeDefault;
}

void UIThemeManager::applyDefaultTheme( UIControl * Control ) {
	if ( mAutoApplyDefaultTheme && NULL != mThemeDefault && NULL != Control )
		Control->setTheme( mThemeDefault );
}

void UIThemeManager::setAutoApplyDefaultTheme( const bool& apply ) {
	mAutoApplyDefaultTheme = apply;
}

const bool& UIThemeManager::getAutoApplyDefaultTheme() const {
	return mAutoApplyDefaultTheme;
}

void UIThemeManager::setDefaultEffectsEnabled( const bool& Enabled ) {
	mEnableDefaultEffects = Enabled;
}

const bool& UIThemeManager::getDefaultEffectsEnabled() const {
	return mEnableDefaultEffects;
}

const Time& UIThemeManager::getControlsFadeInTime() const {
	return mFadeInTime;
}

void UIThemeManager::setControlsFadeInTime( const Time& Time ) {
	mFadeInTime = Time;
}

const Time& UIThemeManager::getControlsFadeOutTime() const {
	return mFadeOutTime;
}

void UIThemeManager::setControlsFadeOutTime( const Time& Time ) {
	mFadeOutTime = Time;
}

void UIThemeManager::setTooltipTimeToShow( const Time& Time ) {
	mtooltipTimeToShow = Time;
}

const Time& UIThemeManager::getTooltipTimeToShow() const {
	return mtooltipTimeToShow;
}

void UIThemeManager::setTooltipFollowMouse( const bool& Follow ) {
	mtooltipFollowMouse = Follow;
}

const bool& UIThemeManager::getTooltipFollowMouse() const {
	return mtooltipFollowMouse;
}

void UIThemeManager::setCursorSize( const Sizei& Size ) {
	mcursorSize = Size;
}

const Sizei& UIThemeManager::getCursorSize() const {
	return mcursorSize;
}

}}
