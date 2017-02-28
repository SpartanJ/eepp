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
	mTooltipTimeToShow( Milliseconds( 200 ) ),
	mTooltipFollowMouse( true ),
	mCursorSize( 16, 16 )
{
}

UIThemeManager::~UIThemeManager() {
}

void UIThemeManager::setDefaultFont( Font * Font ) {
	mFont = Font;

	if ( NULL != mThemeDefault ) {
		FontStyleConfig fontStyleConfig = mThemeDefault->getFontStyleConfig();

		if ( NULL == fontStyleConfig.getFont() && NULL != mFont ) {
			fontStyleConfig.font = mFont;
			mThemeDefault->setFontStyleConfig( fontStyleConfig );
		}
	}
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

	FontStyleConfig fontStyleConfig = mThemeDefault->getFontStyleConfig();

	if ( NULL == fontStyleConfig.getFont() && NULL != mFont ) {
		fontStyleConfig.font = mFont;
		mThemeDefault->setFontStyleConfig( fontStyleConfig );
	}
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
	mTooltipTimeToShow = Time;
}

const Time& UIThemeManager::getTooltipTimeToShow() const {
	return mTooltipTimeToShow;
}

void UIThemeManager::setTooltipFollowMouse( const bool& Follow ) {
	mTooltipFollowMouse = Follow;
}

const bool& UIThemeManager::getTooltipFollowMouse() const {
	return mTooltipFollowMouse;
}

void UIThemeManager::setCursorSize( const Sizei& Size ) {
	mCursorSize = Size;
}

const Sizei& UIThemeManager::getCursorSize() const {
	return mCursorSize;
}

FontStyleConfig UIThemeManager::getDefaultFontStyleConfig() {
	if ( NULL != getDefaultTheme() ) {
		return getDefaultTheme()->getFontStyleConfig();
	}

	return FontStyleConfig();
}

}}
