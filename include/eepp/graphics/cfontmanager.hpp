#ifndef EE_GRAPHICSCFONTMANAGER_HPP
#define EE_GRAPHICSCFONTMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/cfont.hpp>

namespace EE { namespace Graphics {

/** @brief The Font Manager is a singleton class that manages all the instance of fonts instanciated.
	And releases the font instances automatically. So the user doesn't need to release any font instance.
*/
class EE_API cFontManager : public tResourceManager<cFont> {
	SINGLETON_DECLARE_HEADERS(cFontManager)

	public:
		cFontManager();

		virtual ~cFontManager();

		/** @brief Adds a new font to the manager */
		cFont * Add( cFont * Font );
};

}}

#endif
