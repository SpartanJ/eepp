#ifndef EE_GRAPHICS_CGL_HPP
#define EE_GRAPHICS_CGL_HPP

#include "base.hpp"

namespace EE { namespace Graphics {

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
	EEGL_ARB_vertex_buffer_object
};

enum EEGL_version {
	GLv_2,
	GLv_3,
	GLv_ES
};

class cGL {
	static cGL * ms_singleton;
	public:
		static cGL * CreateSingleton();

		static cGL * ExistsSingleton();

		static cGL * instance();

		static void DestroySingleton();

		cGL();

		virtual ~cGL();

		virtual void Init();

		Uint32 GetExtension( const char * name );

		bool IsExtension( Uint32 name );

		bool PointSpriteSupported();

		bool ShadersSupported();

		Uint32 GetTextureParamEnum( const EE_TEXTURE_PARAM& Type );

		Uint32 GetTextureFuncEnum( const EE_TEXTURE_FUNC& Type );

		Uint32 GetTextureSourceEnum( const EE_TEXTURE_SOURCE& Type );

		Uint32 GetTextureOpEnum( const EE_TEXTURE_OP& Type );

		char * GetExtensions();

		void Viewport ( GLint x, GLint y, GLsizei width, GLsizei height );

		void Disable ( GLenum cap );

		void Enable( GLenum cap );

		virtual EEGL_version Version() = 0;

		virtual void PushMatrix() = 0;

		virtual void PopMatrix() = 0;

		virtual void LoadIdentity() = 0;

		virtual void Translatef( GLfloat x, GLfloat y, GLfloat z ) = 0;

		virtual void Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) = 0;

		virtual void Scalef( GLfloat x, GLfloat y, GLfloat z ) = 0;

		virtual void MatrixMode (GLenum mode) = 0;

		virtual void Ortho ( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar ) = 0;

	protected:
		Uint32 mExtensions;
	private:
		void WriteExtension( Uint8 Pos, Uint32 BitWrite );
};

extern cGL * GLi;

}}

#endif
