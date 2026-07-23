#include <eepp/ui/uinode.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UIThemeManager* UIThemeManager::New() {
	return eeNew( UIThemeManager, () );
}

UIThemeManager::UIThemeManager() :
	ResourceManager<UITheme>(),
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
		each( [this]( const auto& resource ) {
			if ( resource.second )
				mResourceScope->removeCatalog( resource.second->getResourceCatalog() );
		} );
	}
}

UITheme* UIThemeManager::add( UITheme* theme ) {
	UITheme* added = ResourceManager<UITheme>::add( theme );
	if ( added && mResourceScope )
		mResourceScope->importCatalog( added->getResourceCatalog() );
	return added;
}

bool UIThemeManager::remove( UITheme* theme, bool destroy ) {
	if ( theme && mResourceScope )
		mResourceScope->removeCatalog( theme->getResourceCatalog() );
	if ( theme == mThemeDefault )
		mThemeDefault = nullptr;
	return ResourceManager<UITheme>::remove( theme, destroy );
}

bool UIThemeManager::removeById( const String::HashType& id, bool destroy ) {
	return remove( getById( id ), destroy );
}

bool UIThemeManager::removeByName( const std::string& name, bool destroy ) {
	return remove( getByName( name ), destroy );
}

UIThemeManager* UIThemeManager::setResourceScope( ResourceScopePtr resourceScope ) {
	if ( mResourceScope == resourceScope )
		return this;

	if ( mResourceScope ) {
		each( [this]( const auto& resource ) {
			if ( resource.second )
				mResourceScope->removeCatalog( resource.second->getResourceCatalog() );
		} );
	}
	mResourceScope = std::move( resourceScope );
	if ( mResourceScope ) {
		each( [this]( const auto& resource ) {
			if ( resource.second )
				mResourceScope->importCatalog( resource.second->getResourceCatalog() );
		} );
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
	UITheme* previousTheme = mThemeDefault;
	if ( previousTheme && previousTheme != Theme && mResourceScope &&
		 !findIf( [previousTheme]( const auto& resource ) {
			 return resource.second == previousTheme;
		 } ) )
		mResourceScope->removeCatalog( previousTheme->getResourceCatalog() );
	mThemeDefault = Theme;
	if ( mThemeDefault && mResourceScope )
		mResourceScope->importCatalog( mThemeDefault->getResourceCatalog() );

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
