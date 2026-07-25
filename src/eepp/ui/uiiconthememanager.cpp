#include <algorithm>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

std::string UIIconThemeManager::getIconNameFromFileName( const std::string& fileName,
														 bool retOnlyWithExtension ) {
	std::string ext( FileSystem::fileExtension( fileName ) );
	if ( !ext.empty() ) {
		return "filetype-" + ext;
	} else if ( !retOnlyWithExtension ) {
		return "filetype-" + String::toLower( fileName );
	}
	return "file";
}

UIIconThemeManager* UIIconThemeManager::New() {
	return eeNew( UIIconThemeManager, () );
}

UIIconThemeManager::~UIIconThemeManager() = default;

UIIconThemeManager::UIIconThemeManager() {}

UIIconThemeManager* UIIconThemeManager::add( UIIconThemePtr iconTheme ) {
	if ( iconTheme && !isPresent( iconTheme.get() ) ) {
		mIconThemes.emplace_back( std::move( iconTheme ) );
	}
	return this;
}

UIIconTheme* UIIconThemeManager::getCurrentTheme() const {
	return mCurrentTheme;
}

UIIconThemeManager* UIIconThemeManager::setCurrentTheme( UIIconThemePtr currentTheme ) {
	if ( currentTheme.get() != mCurrentTheme && currentTheme.get() != mFallbackTheme ) {
		mCurrentTheme = currentTheme.get();
		add( std::move( currentTheme ) );
	}
	return this;
}

UIIconTheme* UIIconThemeManager::getFallbackTheme() const {
	return mFallbackTheme;
}

UIIconThemeManager* UIIconThemeManager::setFallbackTheme( UIIconThemePtr fallbackTheme ) {
	if ( fallbackTheme.get() != mFallbackTheme && fallbackTheme.get() != mCurrentTheme ) {
		mFallbackTheme = fallbackTheme.get();
		add( std::move( fallbackTheme ) );
	}
	return this;
}

UIIcon* UIIconThemeManager::findIcon( const std::string& name ) {
	UIIcon* icon = nullptr;
	if ( mCurrentTheme ) {
		icon = mCurrentTheme->getIcon( name );
		if ( icon )
			return icon;
	}
	if ( mFallbackTheme ) {
		icon = mFallbackTheme->getIcon( name );
		if ( icon )
			return icon;
	}
	if ( mFallbackThemeManager && mFallbackThemeManager->getDefaultTheme() ) {
		return mFallbackThemeManager->getDefaultTheme()->getIconByName( name );
	}
	return nullptr;
}

UIThemeManager* UIIconThemeManager::getFallbackThemeManager() const {
	return mFallbackThemeManager;
}

UIIconThemeManager*
UIIconThemeManager::setFallbackThemeManager( UIThemeManager* fallbackThemeManager ) {
	mFallbackThemeManager = fallbackThemeManager;
	return this;
}

void UIIconThemeManager::remove( UIIconTheme* iconTheme ) {
	auto pos = std::find_if(
		mIconThemes.begin(), mIconThemes.end(),
		[iconTheme]( const UIIconThemePtr& theme ) { return theme.get() == iconTheme; } );
	if ( pos != mIconThemes.end() ) {
		if ( pos->get() == mCurrentTheme ) {
			mCurrentTheme = mFallbackTheme;
			mFallbackTheme = nullptr;
		} else if ( pos->get() == mFallbackTheme ) {
			mFallbackTheme = nullptr;
		}
		mIconThemes.erase( pos );
	}
}

bool UIIconThemeManager::isPresent( UIIconTheme* iconTheme ) {
	return std::find_if( mIconThemes.begin(), mIconThemes.end(),
						 [iconTheme]( const UIIconThemePtr& theme ) {
							 return theme.get() == iconTheme;
						 } ) != mIconThemes.end();
}

}} // namespace EE::UI
