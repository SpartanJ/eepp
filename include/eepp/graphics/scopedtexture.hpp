#ifndef EE_GRAPHICS_SCOPEDTEXTURE_HPP
#define EE_GRAPHICS_SCOPEDTEXTURE_HPP

#include <eepp/config.hpp>

namespace EE { namespace Graphics { namespace Private {

/** @brief binds a texture a restores previusly binded texture */
class EE_API ScopedTexture {
	public:
		ScopedTexture( int textureBind = -1 /** with -1 as default it avoid the auto-binding */ );
		
		~ScopedTexture();
	private:
		int mTextureBinded;
		int mTextureToBind;
};

}}}

#endif
