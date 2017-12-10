#ifndef EE_GRAPHICSGLHELPER_HPP
#define EE_GRAPHICSGLHELPER_HPP

/** Just for reference */
enum EEGL_ARRAY_STATES {
	EEGL_VERTEX_ARRAY			= 0,
	EEGL_NORMAL_ARRAY,
	EEGL_COLOR_ARRAY,
	EEGL_ARRAY_STATES_COUNT,
	EEGL_TEXTURE_COORD_ARRAY
};

enum EEGL_extensions  {
	EEGL_ARB_texture_non_power_of_two	= 0,
	EEGL_ARB_point_parameters,
	EEGL_ARB_point_sprite,
	EEGL_ARB_shading_language_100,
	EEGL_ARB_shader_objects,
	EEGL_ARB_vertex_shader,
	EEGL_ARB_fragment_shader,
	EEGL_EXT_framebuffer_object,
	EEGL_ARB_multitexture,
	EEGL_EXT_texture_compression_s3tc,
	EEGL_ARB_vertex_buffer_object,
	EEGL_ARB_pixel_buffer_object,
	EEGL_ARB_vertex_array_object,
	EEGL_EXT_blend_func_separate,
	EEGL_IMG_texture_compression_pvrtc,
	EEGL_OES_compressed_ETC1_RGB8_texture,
	EEGL_EXT_blend_minmax,
	EEGL_EXT_blend_subtract
};

enum EEGL_version {
	GLv_2,
	GLv_3,
	GLv_3CP,
	GLv_ES1,
	GLv_ES2,
	GLv_default
};

#endif
