#ifndef EE_GRAPHICSCTEXTURESAVER_HPP
#define EE_GRAPHICSCTEXTURESAVER_HPP

#include <eepp/config.hpp>
#include <eepp/graphics/opengl.hpp>

namespace EE { namespace Graphics { namespace Private {

class cTextureSaver {
	public:
		cTextureSaver( int textureBind = -1 /** with -1 as default it avoid the auto-binding */ );
		
		~cTextureSaver();
	private:
		int mTextureBinded;
		int mTextureToBind;
};

}}}

#endif
