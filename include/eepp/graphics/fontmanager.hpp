#ifndef EE_GRAPHICSCFONTMANAGER_HPP
#define EE_GRAPHICSCFONTMANAGER_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/font.hpp>

#include <eepp/system/singleton.hpp>
#include <eepp/system/resourcemanager.hpp>
using namespace EE::System;

namespace EE { namespace Graphics {

/** @brief The Font Manager is a singleton class that manages all the instance of fonts instanciated.
	And releases the font instances automatically. So the user doesn't need to release any font instance.
*/
class EE_API FontManager : public ResourceManager<Font> {
	SINGLETON_DECLARE_HEADERS(FontManager)

	public:
		virtual ~FontManager();

		/** @brief Adds a new font to the manager */
		Graphics::Font * add( Graphics::Font * Font );
	protected:
		FontManager();
};

}}

#endif
