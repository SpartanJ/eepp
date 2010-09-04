#ifndef EE_GRAPHICSGLHELPER_HPP
#define EE_GRAPHICSGLHELPER_HPP

#include "base.hpp"
#include "renders.hpp"

namespace EE { namespace Graphics { namespace Private {

#define EEGL_ARB_texture_non_power_of_two 	(0)
#define EEGL_ARB_point_parameters			(1)
#define EEGL_ARB_point_sprite				(2)
#define EEGL_ARB_shading_language_100		(3)
#define EEGL_ARB_shader_objects				(4)
#define EEGL_ARB_vertex_shader				(5)
#define EEGL_ARB_fragment_shader			(6)
#define EEGL_EXT_framebuffer_object			(7)
#define EEGL_ARB_multitexture				(8)
#define EEGL_EXT_texture_compression_s3tc 	(9)

class cGL : public tSingleton<cGL> {
	friend class tSingleton<cGL>;
	public:
		cGL();

		virtual ~cGL();

		void Init();

		Uint32 GetExtension( const char * name );

		bool IsExtension( Uint32 name );

		bool PointSpriteSupported();

		bool ShadersSupported();

		Uint32 GetTextureParamEnum( const EE_TEXTURE_PARAM& Type );

		Uint32 GetTextureFuncEnum( const EE_TEXTURE_FUNC& Type );

		Uint32 GetTextureSourceEnum( const EE_TEXTURE_SOURCE& Type );

		Uint32 GetTextureOpEnum( const EE_TEXTURE_OP& Type );
	protected:
		Uint32 mExtensions;
	private:
		void WriteExtension( Uint8 Pos, Uint32 BitWrite );
};

}}}

#endif
