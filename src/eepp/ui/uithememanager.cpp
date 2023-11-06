#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UIThemeManager* UIThemeManager::New() {
	return eeNew( UIThemeManager, () );
}

UIThemeManager::UIThemeManager() :
	ResourceManager<UITheme>(),
	mFont( NULL ),
	mFontSize( PixelDensity::dpToPx( PixelDensity::getPixelDensity() > 1.4 ? 11 : 12 ) ),
	mThemeDefault( NULL ),
	mAutoApplyDefaultTheme( true ),
	mEnableDefaultEffects( false ),
	mFadeInTime( Milliseconds( 100.f ) ),
	mFadeOutTime( Milliseconds( 100.f ) ),
	mTooltipTimeToShow( Milliseconds( 400 ) ),
	mTooltipFollowMouse( false ),
	mCursorSize( 16, 16 ) {}

UIThemeManager::~UIThemeManager() {}

UIThemeManager* UIThemeManager::setDefaultFont( Font* Font ) {
	mFont = Font;

	if ( NULL != mFont && NULL != mThemeDefault && NULL == mThemeDefault->getDefaultFont() ) {
		mThemeDefault->setDefaultFont( mFont );
	}

	return this;
}

Font* UIThemeManager::getDefaultFont() const {
	return mFont;
}

UIThemeManager* UIThemeManager::setDefaultFontSize( const Float& fontSize ) {
	mFontSize = fontSize;
	return this;
}

const Float& UIThemeManager::getDefaultFontSize() const {
	return mFontSize;
}

UIThemeManager* UIThemeManager::setDefaultTheme( UITheme* Theme ) {
	mThemeDefault = Theme;

	if ( NULL != mThemeDefault && NULL == mThemeDefault->getDefaultFont() ) {
		setDefaultFont( mFont );
	}
	return this;
}

UIThemeManager* UIThemeManager::setDefaultTheme( const std::string& Theme ) {
	setDefaultTheme( getByName( Theme ) );
	return this;
}

UITheme* UIThemeManager::getDefaultTheme() const {
	return mThemeDefault;
}

UIThemeManager* UIThemeManager::applyDefaultTheme( UINode* node ) {
	if ( mAutoApplyDefaultTheme && NULL != mThemeDefault && NULL != node )
		node->setTheme( mThemeDefault );

	return this;
}

UIThemeManager* UIThemeManager::setAutoApplyDefaultTheme( const bool& apply ) {
	mAutoApplyDefaultTheme = apply;
	return this;
}

const bool& UIThemeManager::getAutoApplyDefaultTheme() const {
	return mAutoApplyDefaultTheme;
}

UIThemeManager* UIThemeManager::setDefaultEffectsEnabled( const bool& Enabled ) {
	mEnableDefaultEffects = Enabled;
	return this;
}

const bool& UIThemeManager::getDefaultEffectsEnabled() const {
	return mEnableDefaultEffects;
}

const Time& UIThemeManager::getWidgetsFadeInTime() const {
	return mFadeInTime;
}

UIThemeManager* UIThemeManager::setWidgetsFadeInTime( const Time& Time ) {
	mFadeInTime = Time;
	return this;
}

const Time& UIThemeManager::getWidgetsFadeOutTime() const {
	return mFadeOutTime;
}

UIThemeManager* UIThemeManager::setWidgetsFadeOutTime( const Time& Time ) {
	mFadeOutTime = Time;
	return this;
}

UIThemeManager* UIThemeManager::setTooltipTimeToShow( const Time& Time ) {
	mTooltipTimeToShow = Time;
	return this;
}

const Time& UIThemeManager::getTooltipTimeToShow() const {
	return mTooltipTimeToShow;
}

UIThemeManager* UIThemeManager::setTooltipFollowMouse( const bool& Follow ) {
	mTooltipFollowMouse = Follow;
	return this;
}

const bool& UIThemeManager::getTooltipFollowMouse() const {
	return mTooltipFollowMouse;
}

UIThemeManager* UIThemeManager::setCursorSize( const Sizei& Size ) {
	mCursorSize = Size;
	return this;
}

const Sizei& UIThemeManager::getCursorSize() const {
	return mCursorSize;
}

}} // namespace EE::UI
