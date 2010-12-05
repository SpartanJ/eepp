#include "cuithememanager.hpp"
#include "cuicontrol.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIThemeManager::cUIThemeManager() :
	tResourceManager<cUITheme>( true ),
	mFont( NULL ),
	mThemeDefault( NULL ),
	mAutoApplyDefaultTheme( true ),
	mEnableDefaultEffects( false ),
	mFadeInTime( 100.f ),
	mFadeOutTime( 100.f )
{
}

cUIThemeManager::~cUIThemeManager() {
}

void cUIThemeManager::DefaultFont( cFont * Font ) {
	mFont = Font;
}

cFont * cUIThemeManager::DefaultFont() const {
	return mFont;
}

void cUIThemeManager::SetTheme( const std::string& Theme ) {
	SetTheme( GetByName( Theme ) );
}

void cUIThemeManager::SetTheme( cUITheme * Theme ) {
	cUIControl * MainCtrl = cUIManager::instance()->MainControl();

	if ( NULL != MainCtrl ) {
		MainCtrl->SetThemeToChilds( Theme );
		MainCtrl->SetTheme( Theme );
	}
}

void cUIThemeManager::DefaultTheme( cUITheme * Theme ) {
	mThemeDefault = Theme;
}

void cUIThemeManager::DefaultTheme( const std::string& Theme ) {
	DefaultTheme( cUIThemeManager::instance()->GetByName( Theme ) );
}

cUITheme * cUIThemeManager::DefaultTheme() const {
	return mThemeDefault;
}

void cUIThemeManager::ApplyDefaultTheme( cUIControl * Control ) {
	if ( mAutoApplyDefaultTheme && NULL != mThemeDefault && NULL != Control )
		Control->SetTheme( mThemeDefault );
}

void cUIThemeManager::AutoApplyDefaultTheme( const bool& apply ) {
	mAutoApplyDefaultTheme = apply;
}

const bool& cUIThemeManager::AutoApplyDefaultTheme() const {
	return mAutoApplyDefaultTheme;
}

void cUIThemeManager::DefaultEffectsEnabled( const bool& Enabled ) {
	mEnableDefaultEffects = Enabled;
}

const bool& cUIThemeManager::DefaultEffectsEnabled() const {
	return mEnableDefaultEffects;
}

const eeFloat& cUIThemeManager::ControlsFadeInTime() const {
	return mFadeInTime;
}

void cUIThemeManager::ControlsFadeInTime( const eeFloat& Time ) {
	mFadeInTime = Time;
}

const eeFloat& cUIThemeManager::ControlsFadeOutTime() const {
	return mFadeOutTime;
}

void cUIThemeManager::ControlsFadeOutTime( const eeFloat& Time ) {
	mFadeOutTime = Time;
}

}}
