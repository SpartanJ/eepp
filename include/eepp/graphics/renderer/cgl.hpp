#ifndef EE_GRAPHICS_CGL_HPP
#define EE_GRAPHICS_CGL_HPP

#include <eepp/graphics/renderer/base.hpp>
#include <eepp/graphics/cshaderprogram.hpp>
#include <eepp/graphics/opengl.hpp>

namespace EE { namespace Graphics {

#ifndef EE_MAX_PLANES
#define EE_MAX_PLANES 6
#endif

class cRendererGL;
class cRendererGL3;
class cRendererGLES2;

/** @brief This class is an abstraction of some OpenGL functionality.
*	eepp have 3 different rendering pipelines: OpenGL 2, OpenGL 3 and OpenGL ES 2. This abstraction is to encapsulate this pipelines.
*	eepp implements its own state machine to simulate fixed-pipeline commands with OpenGL 3 and OpenGL ES 2.
*	Most of the commands can be found in the OpenGL documentation.
*	This is only useful for advanced users that want some control of the OpenGL pipeline. It's mostly used internally by the engine.
*/
class EE_API cGL {
	static cGL * ms_singleton;
	public:
		static cGL * CreateSingleton( EEGL_version ver );

		static cGL * CreateSingleton();

		static cGL * ExistsSingleton();

		static cGL * instance();

		static void DestroySingleton();

		cGL();

		virtual ~cGL();

		virtual void Init();

		/** @return The company responsible for this GL implementation. */
		std::string GetVendor();

		/** @return The name of the renderer.\n This name is typically specific to a particular configuration of a hardware platform. */
		std::string GetRenderer();

		/** @return A GL version or release number. */
		std::string GetVersion();

		/** @return The shading language version */
		std::string GetShadingLanguageVersion();

		/** @return If the extension passed is supported by the GPU */
		bool IsExtension( const std::string& name );

		/** @return If the extension from the EEGL_extensions is present on the GPU. */
		bool IsExtension( EEGL_extensions name );

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

		void DrawArrays (GLenum mode, GLint first, GLsizei count);

		void DrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices );

		void BindTexture ( GLenum target, GLuint texture );

		void ActiveTexture( GLenum texture );

		void BlendFunc ( GLenum sfactor, GLenum dfactor );

		void Viewport ( GLint x, GLint y, GLsizei width, GLsizei height );

		void LineSmooth( const bool& Enable );

		/** Reapply the line smooth state */
		void LineSmooth();

		bool IsLineSmooth();

		/** Set the polygon fill mode ( wireframe or filled ) */
		void PolygonMode( const EE_FILL_MODE& Mode );

		/** Reapply the polygon mode */
		void PolygonMode();

		cRendererGL * GetRendererGL();

		cRendererGL3 * GetRendererGL3();

		cRendererGLES2 * GetRendererGLES2();

		virtual void PointSize( GLfloat size ) = 0;

		virtual GLfloat PointSize() = 0;

		virtual void ClientActiveTexture( GLenum texture ) = 0;

		virtual void Disable ( GLenum cap );

		virtual void Enable( GLenum cap );

		virtual EEGL_version Version() = 0;

		virtual std::string VersionStr() = 0;

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

		virtual void VertexPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) = 0;

		virtual void ColorPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) = 0;

		virtual void TexCoordPointer ( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer ) = 0;

		virtual void SetShader( cShaderProgram * Shader );

		virtual void Clip2DPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height ) = 0;

		virtual void Clip2DPlaneDisable() = 0;

		virtual void MultMatrixf ( const GLfloat *m ) = 0;

		virtual void ClipPlane( GLenum plane, const GLdouble *equation ) = 0;

		virtual void TexEnvi( GLenum target, GLenum pname, GLint param ) = 0;

		virtual void LoadMatrixf( const GLfloat *m ) = 0;

		virtual void Frustum( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat near_val, GLfloat far_val ) = 0;

		virtual void GetCurrentMatrix( GLenum mode, GLfloat * m ) = 0;

		virtual GLenum GetCurrentMatrixMode() = 0;

		void GetViewport( GLint * viewport );

		virtual GLint Project( GLfloat objx, GLfloat objy, GLfloat objz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *winx, GLfloat *winy, GLfloat *winz ) = 0;

		virtual GLint UnProject( GLfloat winx, GLfloat winy, GLfloat winz, const GLfloat modelMatrix[16], const GLfloat projMatrix[16], const GLint viewport[4], GLfloat *objx, GLfloat *objy, GLfloat *objz ) = 0;

		eeVector3f ProjectCurrent( const eeVector3f& point );

		eeVector3f UnProjectCurrent( const eeVector3f& point );
	protected:
		enum GLStateFlags {
			GLSF_LINE_SMOOTH	= 0,
			GLSF_POLYGON_MODE
		};

		Uint32 mExtensions;
		Uint32 mStateFlags;
	private:
		void WriteExtension( Uint8 Pos, Uint32 BitWrite );
};

extern EE_API cGL * GLi;

}}

#endif
