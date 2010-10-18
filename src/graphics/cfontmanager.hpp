#ifndef EE_GRAPHICSCFONTMANAGER_HPP
#define EE_GRAPHICSCFONTMANAGER_HPP

#include "base.hpp"
#include "cfont.hpp"

namespace EE { namespace Graphics {

class EE_API cFontManager : public tResourceManager<cFont>, public tSingleton<cFontManager> {
	friend class tSingleton<cFontManager>;
	public:
		cFontManager();

		virtual ~cFontManager();

		cFont * Add( cFont * Font );
	protected:
};

}}

#endif
