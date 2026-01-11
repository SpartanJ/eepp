#ifndef EE_RENDER_MODE_HPP
#define EE_RENDER_MODE_HPP

namespace EE { namespace Graphics {

/** @enum RenderMode Defines the method to use to render a texture. */
enum RenderMode {
	RENDER_NORMAL = 0,			   //!< Render the texture without any change
	RENDER_MIRROR = 1,			   //!< Render the texture mirrored
	RENDER_FLIPPED = 2,			   //!< Render the texture flipped
	RENDER_FLIPPED_MIRRORED = 3,   //!< Render the texture flipped and mirrored
	RENDER_ISOMETRIC = 4,		   //!< Render the texture as an isometric tile
	RENDER_ISOMETRIC_VERTICAL = 5, //!< Render the texture as an isometric vertical tile
	RENDER_ISOMETRIC_VERTICAL_NEGATIVE =
		6 //!< Render the texture as an isometric vectical tile mirrored
};

}} // namespace EE::Graphics

#endif
