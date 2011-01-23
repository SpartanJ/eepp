#ifndef EE_GRAPHICS_CGL_HPP
#define EE_GRAPHICS_CGL_HPP

#include "base.hpp"
#include "../cshaderprogram.hpp"

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

/** Just for reference */
enum EEGL_ARRAY_STATES {
	EEGL_VERTEX_ARRAY			= 0,
	EEGL_NORMAL_ARRAY			= 1,
	EEGL_COLOR_ARRAY			= 2,
	EEGL_INDEX_ARRAY			= 3,
	EEGL_TEXTURE_COORD_ARRAY	= 4,
	EEGL_EDGE_FLAG_ARRAY		= 5,
	EEGL_ARRAY_STATES_SIZE
};

enum EEGL_SHADERS_NUM {
	EEGL_SHADER_BASE_TEX,
	EEGL_SHADER_BASE,
	EEGL_SHADER_POINT_SPRITE,
	EEGL_SHADERS_COUNT
};

enum EEGL_version {
	GLv_2,
	GLv_3,
	GLv_ES,
	GLv_default
};

class cRendererGL;
class cRendererGL3;

class cGL {
	static cGL * ms_singleton;
	public:
		static cGL * CreateSingleton();

		static cGL * CreateSingleton( EEGL_version ver );

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

		void Clear ( GLbitfield mask );

		void ClearColor ( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );

		void Scissor ( GLint x, GLint y, GLsizei width, GLsizei height );

		void PolygonMode( GLenum face, GLenum mode );

		char * GetExtensions();

		char * GetString( GLenum name );

		void PointSize( GLfloat size );

		void DrawArrays (GLenum mode, GLint first, GLsizei count);

		void DrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );

		void BindTexture ( GLenum target, GLuint texture );

		void ActiveTexture( GLenum texture );

		void BlendFunc ( GLenum sfactor, GLenum dfactor );

		void Viewport ( GLint x, GLint y, GLsizei width, GLsizei height );

		virtual void Disable ( GLenum cap );

		virtual void Enable( GLenum cap );

		virtual EEGL_version Version() = 0;

		virtual void PushMatrix() = 0;

		virtual void PopMatrix() = 0;

		virtual void LoadIdentity() = 0;

		virtual void Translatef( GLfloat x, GLfloat y, GLfloat z ) = 0;

		virtual void Rotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z ) = 0;

		virtual void Scalef( GLfloat x, GLfloat y, GLfloat z ) = 0;

		virtual void MatrixMode ( GLenum mode ) = 0;

		virtual void Ortho ( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar ) = 0;

		virtual void LookAt( GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ ) = 0;

		virtual void Perspective ( GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar ) = 0;

		virtual void EnableClientState( GLenum array ) = 0;

		virtual void DisableClientState( GLenum array ) = 0;

		virtual void VertexPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) = 0;

		virtual void ColorPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) = 0;

		virtual void TexCoordPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer, GLuint allocate ) = 0;

		virtual void SetShader( cShaderProgram * Shader );

		cRendererGL * GetRendererGL();

		cRendererGL3 * GetRendererGL3();
	protected:
		Uint32 mExtensions;
	private:
		void WriteExtension( Uint8 Pos, Uint32 BitWrite );
};

extern cGL * GLi;

}}

#endif
