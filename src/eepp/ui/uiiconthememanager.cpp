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

UIIconThemeManager::~UIIconThemeManager() {
	for ( UIIconTheme* theme : mIconThemes )
		eeDelete( theme );
}

UIIconThemeManager::UIIconThemeManager() {}

UIIconThemeManager* UIIconThemeManager::add( UIIconTheme* iconTheme ) {
	if ( !isPresent( iconTheme ) ) {
		mIconThemes.push_back( iconTheme );
	}
	return this;
}

UIIconTheme* UIIconThemeManager::getCurrentTheme() const {
	return mCurrentTheme;
}

UIIconThemeManager* UIIconThemeManager::setCurrentTheme( UIIconTheme* currentTheme ) {
	if ( currentTheme != mCurrentTheme && currentTheme != mFallbackTheme ) {
		if ( !isPresent( currentTheme ) )
			add( currentTheme );
		mCurrentTheme = currentTheme;
	}
	return this;
}

UIIconTheme* UIIconThemeManager::getFallbackTheme() const {
	return mFallbackTheme;
}

UIIconThemeManager* UIIconThemeManager::setFallbackTheme( UIIconTheme* fallbackTheme ) {
	if ( fallbackTheme != mFallbackTheme && fallbackTheme != mCurrentTheme ) {
		if ( !isPresent( fallbackTheme ) )
			add( fallbackTheme );
		mFallbackTheme = fallbackTheme;
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
	auto pos = std::find( mIconThemes.begin(), mIconThemes.end(), iconTheme );
	if ( pos != mIconThemes.end() ) {
		if ( *pos == mCurrentTheme ) {
			mCurrentTheme = mFallbackTheme;
			mFallbackTheme = nullptr;
		} else if ( *pos == mFallbackTheme ) {
			mFallbackTheme = nullptr;
		}
		mIconThemes.erase( pos );
	}
}

bool UIIconThemeManager::isPresent( UIIconTheme* iconTheme ) {
	return std::find( mIconThemes.begin(), mIconThemes.end(), iconTheme ) != mIconThemes.end();
}

}} // namespace EE::UI
