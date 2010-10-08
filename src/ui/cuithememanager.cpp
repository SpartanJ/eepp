#include "cuithememanager.hpp"

namespace EE { namespace UI {

cUIThemeManager::cUIThemeManager() :
	tResourceManager<cUITheme>( true )
{
}

cUIThemeManager::~cUIThemeManager() {
	
}

}} 
