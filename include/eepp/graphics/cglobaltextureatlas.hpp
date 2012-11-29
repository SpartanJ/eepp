#ifndef EE_GRAPHICSCGLOBALTEXTUREATLAS_HPP
#define EE_GRAPHICSCGLOBALTEXTUREATLAS_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/ctextureatlas.hpp>

namespace EE { namespace Graphics {

class EE_API cGlobalTextureAtlas : public cTextureAtlas {
	SINGLETON_DECLARE_HEADERS(cGlobalTextureAtlas)

	public:
		cGlobalTextureAtlas();

		~cGlobalTextureAtlas();
};

}}

#endif
