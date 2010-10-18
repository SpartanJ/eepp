#include "cuithememanager.hpp"

namespace EE { namespace UI {

cUIThemeManager::cUIThemeManager() :
	tResourceManager<cUITheme>( true ),
	mFont( NULL )
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

}} 
