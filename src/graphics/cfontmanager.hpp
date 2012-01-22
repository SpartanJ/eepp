#ifndef EE_GRAPHICSCFONTMANAGER_HPP
#define EE_GRAPHICSCFONTMANAGER_HPP

#include "base.hpp"
#include "cfont.hpp"

namespace EE { namespace Graphics {

class EE_API cFontManager : public tResourceManager<cFont> {
	SINGLETON_DECLARE_HEADERS(cFontManager)

	public:
		cFontManager();

		virtual ~cFontManager();

		cFont * Add( cFont * Font );
	protected:
};

}}

#endif
