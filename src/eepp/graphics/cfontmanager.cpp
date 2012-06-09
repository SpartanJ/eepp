#include <eepp/graphics/cfontmanager.hpp>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(cFontManager)

cFontManager::cFontManager() {
}

cFontManager::~cFontManager() {
}

cFont * cFontManager::Add( cFont * Font ) {
	eeASSERT( NULL != Font );
	return tResourceManager<cFont>::Add( Font );
}

}}
