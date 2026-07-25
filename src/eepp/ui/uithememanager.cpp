#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UIThemeManager* UIThemeManager::New() {
	return eeNew( UIThemeManager, () );
}

UIThemeManager::UIThemeManager() :
	mFont( NULL ),
	mFontSize( PixelDensity::dpToPx( 12 ) ),
	mThemeDefault( NULL ),
	mAutoApplyDefaultTheme( true ),
	mEnableDefaultEffects( false ),
	mFadeInTime( Milliseconds( 25.f ) ),
	mFadeOutTime( Milliseconds( 25.f ) ),
	mTooltipTimeToShow( Milliseconds( 400 ) ),
	mTooltipFollowMouse( false ),
	mCursorSize( 16, 16 ) {}

UIThemeManager::~UIThemeManager() {
	if ( mResourceScope ) {
		for ( const auto& resource : mThemes )
			mResourceScope->removeCatalog( resource.second->getResourceCatalog() );
		if ( mThemeDefault && mThemes.find( mThemeDefault->getId() ) == mThemes.end() )
			mResourceScope->removeCatalog( mThemeDefault->getResourceCatalog() );
	}
}

UITheme* UIThemeManager::add( UIThemePtr theme ) {
	if ( !theme )
		return nullptr;
	UITheme* added = theme.get();
	auto existing = mThemes.find( theme->getId() );
	const bool replacesDefault =
		existing != mThemes.end() && existing->second.get() == mThemeDefault.get();
	if ( existing != mThemes.end() && mResourceScope )
		mResourceScope->removeCatalog( existing->second->getResourceCatalog() );
	mThemes[theme->getId()] = std::move( theme );
	if ( replacesDefault )
		mThemeDefault = mThemes[added->getId()];
	if ( mResourceScope )
		mResourceScope->importCatalog( added->getResourceCatalog() );
	return added;
}

bool UIThemeManager::remove( UITheme* theme ) {
	if ( !theme )
		return false;
	auto it = mThemes.find( theme->getId() );
	const bool isManaged = it != mThemes.end() && it->second.get() == theme;
	const bool isDefault = theme == mThemeDefault.get();
	if ( !isManaged && !isDefault )
		return false;
	if ( mResourceScope )
		mResourceScope->removeCatalog( theme->getResourceCatalog() );
	if ( isDefault )
		mThemeDefault.reset();
	if ( isManaged )
		mThemes.erase( it );
	return true;
}

bool UIThemeManager::removeById( const String::HashType& id ) {
	return remove( getById( id ) );
}

bool UIThemeManager::removeByName( const std::string& name ) {
	return remove( getByName( name ) );
}

UIThemeManager* UIThemeManager::setResourceScope( ResourceScopePtr resourceScope ) {
	if ( mResourceScope == resourceScope )
		return this;

	if ( mResourceScope ) {
		for ( const auto& resource : mThemes )
			mResourceScope->removeCatalog( resource.second->getResourceCatalog() );
		if ( mThemeDefault && mThemes.find( mThemeDefault->getId() ) == mThemes.end() )
			mResourceScope->removeCatalog( mThemeDefault->getResourceCatalog() );
	}
	mResourceScope = std::move( resourceScope );
	if ( mResourceScope ) {
		for ( const auto& resource : mThemes )
			mResourceScope->importCatalog( resource.second->getResourceCatalog() );
		if ( mThemeDefault && mThemes.find( mThemeDefault->getId() ) == mThemes.end() )
			mResourceScope->importCatalog( mThemeDefault->getResourceCatalog() );
	}
	return this;
}

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
	UITheme* previousTheme = mThemeDefault.get();
	if ( previousTheme && previousTheme != Theme && mResourceScope &&
		 mThemes.find( previousTheme->getId() ) == mThemes.end() )
		mResourceScope->removeCatalog( previousTheme->getResourceCatalog() );
	if ( Theme ) {
		auto it = mThemes.find( Theme->getId() );
		mThemeDefault =
			it != mThemes.end() && it->second.get() == Theme ? it->second : UIThemePtr{};
	} else {
		mThemeDefault.reset();
	}
	if ( mThemeDefault && mResourceScope )
		mResourceScope->importCatalog( mThemeDefault->getResourceCatalog() );

	if ( NULL != mThemeDefault && NULL == mThemeDefault->getDefaultFont() ) {
		setDefaultFont( mFont );
	}
	return this;
}

UIThemeManager* UIThemeManager::setDefaultTheme( UIThemePtr theme ) {
	UITheme* previousTheme = mThemeDefault.get();
	if ( previousTheme && previousTheme != theme.get() && mResourceScope &&
		 mThemes.find( previousTheme->getId() ) == mThemes.end() )
		mResourceScope->removeCatalog( previousTheme->getResourceCatalog() );
	mThemeDefault = std::move( theme );
	if ( mThemeDefault && mResourceScope )
		mResourceScope->importCatalog( mThemeDefault->getResourceCatalog() );
	if ( mThemeDefault && !mThemeDefault->getDefaultFont() )
		mThemeDefault->setDefaultFont( mFont );
	return this;
}

UIThemeManager* UIThemeManager::setDefaultTheme( const std::string& Theme ) {
	setDefaultTheme( getByName( Theme ) );
	return this;
}

UITheme* UIThemeManager::getDefaultTheme() const {
	return mThemeDefault.get();
}

UIThemePtr UIThemeManager::getDefaultThemeHandle() const {
	return mThemeDefault;
}

UITheme* UIThemeManager::getById( const String::HashType& id ) const {
	auto it = mThemes.find( id );
	return it != mThemes.end() ? it->second.get() : nullptr;
}

UITheme* UIThemeManager::getByName( const std::string& name ) const {
	return getById( String::hash( name ) );
}

UIThemeManager* UIThemeManager::applyDefaultTheme( UINode* node ) {
	if ( mAutoApplyDefaultTheme && NULL != mThemeDefault && NULL != node )
		node->setTheme( mThemeDefault.get() );

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
