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

void UIThemeManager::DefaultFont( Font * Font ) {
	mFont = Font;
}

Font * UIThemeManager::DefaultFont() const {
	return mFont;
}

void UIThemeManager::SetTheme( const std::string& Theme ) {
	SetTheme( getByName( Theme ) );
}

void UIThemeManager::SetTheme( UITheme * Theme ) {
	UIControl * MainCtrl = UIManager::instance()->MainControl();

	if ( NULL != MainCtrl ) {
		MainCtrl->SetThemeToChilds( Theme );
		MainCtrl->SetTheme( Theme );
	}
}

void UIThemeManager::DefaultTheme( UITheme * Theme ) {
	mThemeDefault = Theme;
}

void UIThemeManager::DefaultTheme( const std::string& Theme ) {
	DefaultTheme( UIThemeManager::instance()->getByName( Theme ) );
}

UITheme * UIThemeManager::DefaultTheme() const {
	return mThemeDefault;
}

void UIThemeManager::ApplyDefaultTheme( UIControl * Control ) {
	if ( mAutoApplyDefaultTheme && NULL != mThemeDefault && NULL != Control )
		Control->SetTheme( mThemeDefault );
}

void UIThemeManager::AutoApplyDefaultTheme( const bool& apply ) {
	mAutoApplyDefaultTheme = apply;
}

const bool& UIThemeManager::AutoApplyDefaultTheme() const {
	return mAutoApplyDefaultTheme;
}

void UIThemeManager::DefaultEffectsEnabled( const bool& Enabled ) {
	mEnableDefaultEffects = Enabled;
}

const bool& UIThemeManager::DefaultEffectsEnabled() const {
	return mEnableDefaultEffects;
}

const Time& UIThemeManager::ControlsFadeInTime() const {
	return mFadeInTime;
}

void UIThemeManager::ControlsFadeInTime( const Time& Time ) {
	mFadeInTime = Time;
}

const Time& UIThemeManager::ControlsFadeOutTime() const {
	return mFadeOutTime;
}

void UIThemeManager::ControlsFadeOutTime( const Time& Time ) {
	mFadeOutTime = Time;
}

void UIThemeManager::TooltipTimeToShow( const Time& Time ) {
	mTooltipTimeToShow = Time;
}

const Time& UIThemeManager::TooltipTimeToShow() const {
	return mTooltipTimeToShow;
}

void UIThemeManager::TooltipFollowMouse( const bool& Follow ) {
	mTooltipFollowMouse = Follow;
}

const bool& UIThemeManager::TooltipFollowMouse() const {
	return mTooltipFollowMouse;
}

void UIThemeManager::CursorSize( const Sizei& Size ) {
	mCursorSize = Size;
}

const Sizei& UIThemeManager::CursorSize() const {
	return mCursorSize;
}

}}
