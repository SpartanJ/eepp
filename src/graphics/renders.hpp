#ifndef EERENDERS_H
#define EERENDERS_H

namespace EE { namespace Graphics {

/** @enum EE_FILLMODE Defines the fill mode for the primitives. */
typedef enum {
	DRAW_LINE, //!< Draw only lines
	DRAW_FILL //!< Draw filled objects
} EE_FILLMODE;

/** @enum EE_TEX_FILTER Defines the texture filter used. */
typedef enum {
	TEX_LINEAR, //!< Linear filtering (Smoothed Zoom)
	TEX_NEAREST //!< No filtering (Pixeled Zoom)
} EE_TEX_FILTER;

/** @enum EE_RENDERALPHAS Defines the Blend Function to use */
typedef enum {
	ALPHA_NONE, //!< Disable the GL_BLEND
	ALPHA_NORMAL, //!< glBlendFunc(GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA);
	ALPHA_BLENDONE, //!< glBlendFunc(GL_SRC_ALPHA , GL_ONE);
	ALPHA_BLENDTWO, //!< glBlendFunc(GL_SRC_ALPHA , GL_SRC_ALPHA); \n glBlendFunc(GL_DST_ALPHA , GL_ONE);
	ALPHA_BLENDTHREE, //!< glBlendFunc(GL_SRC_ALPHA , GL_ONE); \n glBlendFunc(GL_DST_ALPHA , GL_SRC_ALPHA);
	ALPHA_ALPHACHANNELS, //!< glBlendFunc(GL_SRC_ALPHA , GL_SRC_ALPHA); 
	ALPHA_DESTALPHA, //!< glBlendFunc(GL_SRC_ALPHA , GL_DST_ALPHA);
	ALPHA_MULTIPLY //!< glBlendFunc(GL_DST_COLOR,GL_ZERO);
} EE_RENDERALPHAS;

/** @enum EE_RENDERTYPE Defines the method to use to render a texture. */
typedef enum {
	RN_NORMAL = 0, //!< Render the texture without any change
	RN_MIRROR = 1, //!< Render the texture mirrored
	RN_FLIP = 2, //!< Render the texture fliped
	RN_FLIPMIRROR = 3, //!< Render the texture fliped and mirrored
	RN_ISOMETRIC = 4, //!< Render the texture as an isometric tile 
	RN_ISOMETRICVERTICAL = 5, //!< Render the texture as an isometric vertical tile
	RN_ISOMETRICVERTICALNEGATIVE = 6 //!< Render the texture as an isometric vectical tile mirrored
} EE_RENDERTYPE;

/** @enum EE_SAVEFORMAT Defines the format to save a texture. */
typedef enum {
	EE_SAVE_TYPE_TGA = 0,
	EE_SAVE_TYPE_BMP = 1,
	EE_SAVE_TYPE_DDS = 2
} EE_SAVETYPE;

/** @enum EE_TTF_FONTSTYLE Set the TTF Font style. */
typedef enum {
	EE_TTF_STYLE_NORMAL = 0,
	EE_TTF_STYLE_BOLD = 1,
	EE_TTF_STYLE_ITALIC = 2,
	EE_TTF_STYLE_UNDERLINE = 4
} EE_TTF_FONTSTYLE;

/** @enum EE_CLAMP_MODE Set the clamp mode of the texture. */
typedef enum {
	EE_CLAMP_TO_EDGE,
	EE_CLAMP_REPEAT
} EE_CLAMP_MODE;

/** @enum EE_BATCH_RENDERER_METHOD The batch renderer, rendering methods */
typedef enum  {
	EE_GL_POINTS = 0x0000,
	EE_GL_LINES = 0x0001,
	EE_GL_LINE_LOOP = 0x0002,
	EE_GL_LINE_STRIP = 0x0003,
	EE_GL_TRIANGLES = 0x0004,
	EE_GL_TRIANGLE_STRIP = 0x0005,
	EE_GL_TRIANGLE_FAN = 0x0006,
	EE_GL_QUADS = 0x0007,
	EE_GL_QUAD_STRIP = 0x0008,
	EE_GL_POLYGON = 0x0009
} EE_BATCH_RENDER_METHOD;

}

}

#endif
