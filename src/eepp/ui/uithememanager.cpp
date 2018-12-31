#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uinode.hpp>

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

UIThemeManager *  UIThemeManager::setDefaultFont( Font * Font ) {
	mFont = Font;

	if ( NULL != mThemeDefault ) {
		UIFontStyleConfig fontStyleConfig = mThemeDefault->getFontStyleConfig();

		if ( NULL == fontStyleConfig.getFont() && NULL != mFont ) {
			fontStyleConfig.Font = mFont;
			mThemeDefault->setFontStyleConfig( fontStyleConfig );
		}
	}

	return this;
}

Font * UIThemeManager::getDefaultFont() const {
	return mFont;
}

UIThemeManager *  UIThemeManager::setDefaultTheme( UITheme * Theme ) {
	mThemeDefault = Theme;

	if ( NULL != mThemeDefault ) {
		UITooltipStyleConfig fontStyleConfig = mThemeDefault->getFontStyleConfig();

		if ( NULL == fontStyleConfig.getFont() && NULL != mFont ) {
			fontStyleConfig.Font = mFont;
			mThemeDefault->setFontStyleConfig( fontStyleConfig );
		}
	}
	return this;
}

UIThemeManager *  UIThemeManager::setDefaultTheme( const std::string& Theme ) {
	setDefaultTheme( UIThemeManager::instance()->getByName( Theme ) );
	return this;
}

UITheme * UIThemeManager::getDefaultTheme() const {
	return mThemeDefault;
}

UIThemeManager *  UIThemeManager::applyDefaultTheme( UINode * Control ) {
	if ( mAutoApplyDefaultTheme && NULL != mThemeDefault && NULL != Control )
		Control->setTheme( mThemeDefault );

	return this;
}

UIThemeManager *  UIThemeManager::setAutoApplyDefaultTheme( const bool& apply ) {
	mAutoApplyDefaultTheme = apply;
	return this;
}

const bool& UIThemeManager::getAutoApplyDefaultTheme() const {
	return mAutoApplyDefaultTheme;
}

UIThemeManager *  UIThemeManager::setDefaultEffectsEnabled( const bool& Enabled ) {
	mEnableDefaultEffects = Enabled;
	return this;
}

const bool& UIThemeManager::getDefaultEffectsEnabled() const {
	return mEnableDefaultEffects;
}

const Time& UIThemeManager::getControlsFadeInTime() const {
	return mFadeInTime;
}

UIThemeManager *  UIThemeManager::setControlsFadeInTime( const Time& Time ) {
	mFadeInTime = Time;
	return this;
}

const Time& UIThemeManager::getControlsFadeOutTime() const {
	return mFadeOutTime;
}

UIThemeManager *  UIThemeManager::setControlsFadeOutTime( const Time& Time ) {
	mFadeOutTime = Time;
	return this;
}

UIThemeManager *  UIThemeManager::setTooltipTimeToShow( const Time& Time ) {
	mTooltipTimeToShow = Time;
	return this;
}

const Time& UIThemeManager::getTooltipTimeToShow() const {
	return mTooltipTimeToShow;
}

UIThemeManager *  UIThemeManager::setTooltipFollowMouse( const bool& Follow ) {
	mTooltipFollowMouse = Follow;
	return this;
}

const bool& UIThemeManager::getTooltipFollowMouse() const {
	return mTooltipFollowMouse;
}

UIThemeManager *  UIThemeManager::setCursorSize( const Sizei& Size ) {
	mCursorSize = Size;
	return this;
}

const Sizei& UIThemeManager::getCursorSize() const {
	return mCursorSize;
}

}}
