#ifndef EERENDERS_H
#define EERENDERS_H

namespace EE { namespace Graphics {

#define EE_MAX_TEXTURE_UNITS 4

/** @enum EE_FILL_MODE Defines the fill mode for the primitives. */
enum EE_FILL_MODE {
	EE_DRAW_LINE, 				//!< Draw only lines
	EE_DRAW_FILL 				//!< Draw filled objects
};

/** @enum EE_TEX_FILTER Defines the texture filter used. */
enum EE_TEX_FILTER {
	TEX_FILTER_LINEAR, 			//!< Linear filtering (Smoothed Zoom)
	TEX_FILTER_NEAREST 			//!< No filtering (Pixeled Zoom)
};

/** @enum EE_BLEND_MODE Predefined blend modes */
enum EE_BLEND_MODE {
	ALPHA_NONE, 			//!< Disable the blend
	ALPHA_NORMAL, 			//!< src SRC_ALPHA dst ONE_MINUS_SRC_ALPHA
	ALPHA_BLENDONE, 		//!< src SRC_ALPHA dst ONE
	ALPHA_BLENDTWO, 		//!< src SRC_ALPHA dst SRC_ALPHA \n src DST_ALPHA dst ONE
	ALPHA_BLENDTHREE, 		//!< src SRC_ALPHA dst ONE \n src DST_ALPHA dst SRC_ALPHA
	ALPHA_ALPHACHANNELS, 	//!< src SRC_ALPHA dst SRC_ALPHA
	ALPHA_DESTALPHA, 		//!< src SRC_ALPHA dst DST_ALPHA
	ALPHA_MULTIPLY, 		//!< src DST_COLOR dst ZERO
	ALPHA_CUSTOM			//!< Disable the Predefined blend mode for the use of custom blend funcs.
};

/** @enum EE_RENDER_MODE Defines the method to use to render a texture. */
enum EE_RENDER_MODE {
	RN_NORMAL 						= 0, 	//!< Render the texture without any change
	RN_MIRROR 						= 1, 	//!< Render the texture mirrored
	RN_FLIP 						= 2, 	//!< Render the texture fliped
	RN_FLIPMIRROR 					= 3, 	//!< Render the texture fliped and mirrored
	RN_ISOMETRIC 					= 4, 	//!< Render the texture as an isometric tile
	RN_ISOMETRICVERTICAL 			= 5, 	//!< Render the texture as an isometric vertical tile
	RN_ISOMETRICVERTICALNEGATIVE 	= 6 	//!< Render the texture as an isometric vectical tile mirrored
};

/** @enum EE_SAVE_TYPE Defines the format to save a texture. */
enum EE_SAVE_TYPE {
	EE_SAVE_TYPE_UNKNOWN	= -1,
	EE_SAVE_TYPE_TGA 		= 0,
	EE_SAVE_TYPE_BMP 		= 1,
	EE_SAVE_TYPE_PNG 		= 2,
	EE_SAVE_TYPE_DDS 		= 3,
	EE_SAVE_TYPE_JPG		= 4
};

/** @enum EE_TTF_FONT_STYLE Set the TTF Font style. */
enum EE_TTF_FONT_STYLE {
	EE_TTF_STYLE_NORMAL 	= 0,
	EE_TTF_STYLE_BOLD 		= 1,
	EE_TTF_STYLE_ITALIC 	= 2,
	EE_TTF_STYLE_UNDERLINE 	= 4
};

/** @enum EE_CLAMP_MODE Set the clamp mode of the texture. */
enum EE_CLAMP_MODE {
	EE_CLAMP_TO_EDGE,
	EE_CLAMP_REPEAT
};

/** @enum EE_DRAW_MODE The batch renderer, rendering methods */
enum EE_DRAW_MODE {
	DM_POINTS 			= 0x0000,
	DM_LINES 			= 0x0001,
	DM_LINE_LOOP 		= 0x0002,
	DM_LINE_STRIP 		= 0x0003,
	DM_TRIANGLES 		= 0x0004,
	DM_TRIANGLE_STRIP 	= 0x0005,
	DM_TRIANGLE_FAN 	= 0x0006,
	DM_QUADS 			= 0x0007,
	DM_QUAD_STRIP 		= 0x0008,
	DM_POLYGON 			= 0x0009
};

/** @enum EE_BLEND_FUNC Blend functions */
enum EE_BLEND_FUNC {
	BF_ZERO					= 0,
	BF_ONE					= 1,
	BF_SRC_COLOR			= 0x0300,
	BF_ONE_MINUS_SRC_COLOR	= 0x0301,
	BF_SRC_ALPHA			= 0x0302,
	BF_ONE_MINUS_SRC_ALPHA	= 0x0303,
	BF_DST_ALPHA			= 0x0304,
	BF_ONE_MINUS_DST_ALPHA	= 0x0305,
	BF_DST_COLOR			= 0x0306,
	BF_ONE_MINUS_DST_COLOR	= 0x0307,
	BF_SRC_ALPHA_SATURATE	= 0x0308
};

/** @enum EE_TEXTURE_PARAM Texture Params */
enum EE_TEXTURE_PARAM {
	TEX_PARAM_COLOR_FUNC,
	TEX_PARAM_ALPHA_FUNC,
	TEX_PARAM_COLOR_SOURCE_0,
	TEX_PARAM_COLOR_SOURCE_1,
	TEX_PARAM_COLOR_SOURCE_2,
	TEX_PARAM_ALPHA_SOURCE_0,
	TEX_PARAM_ALPHA_SOURCE_1,
	TEX_PARAM_ALPHA_SOURCE_2,
	TEX_PARAM_COLOR_OP_0,
	TEX_PARAM_COLOR_OP_1,
	TEX_PARAM_COLOR_OP_2,
	TEX_PARAM_ALPHA_OP_0,
	TEX_PARAM_ALPHA_OP_1,
	TEX_PARAM_ALPHA_OP_2,
	TEX_PARAM_COLOR_SCALE,
	TEX_PARAM_ALPHA_SCALE
};

/** @enum EE_TEXTURE_OP Texture Env Op */
enum EE_TEXTURE_OP {
	TEX_OP_COLOR,
	TEX_OP_ONE_MINUS_COLOR,
	TEX_OP_ALPHA,
	TEX_OP_ONE_MINUS_ALPHA
};

/** @enum EE_TEXTURE_FUNC Texture functions */
enum EE_TEXTURE_FUNC {
	TEX_FUNC_MODULATE,
	TEX_FUNC_REPLACE,
	TEX_FUNC_ADD,
	TEX_FUNC_SUBSTRACT,
	TEX_FUNC_ADD_SIGNED,
	TEX_FUNC_INTERPOLATE,
	TEX_FUNC_DOT3_RGB,
	TEX_FUNC_DOT3_RGBA
};

/** @enum EE_ALPHA_TEST_FUNC Alpha test functions */
enum EE_ALPHA_TEST_FUNC {
	ALPHA_FUNC_NEVER,
	ALPHA_FUNC_LESS,
	ALPHA_FUNC_LEQUAL,
	ALPHA_FUNC_GREATER,
	ALPHA_FUNC_GEQUAL,
	ALPHA_FUNC_EQUAL,
	ALPHA_FUNC_NOTEQUAL,
	ALPHA_FUNC_ALWAYS
};

/** @enum EE_TEXTURE_SOURCE The Texture Source */
enum EE_TEXTURE_SOURCE {
	TEX_SRC_TEXTURE,
	TEX_SRC_CONSTANT,
	TEX_SRC_PRIMARY,
	TEX_SRC_PREVIOUS
};

}}

#endif
