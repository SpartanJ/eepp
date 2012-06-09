#ifndef EE_GRAPHICSCFONTMANAGER_HPP
#define EE_GRAPHICSCFONTMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cfont.hpp>

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
